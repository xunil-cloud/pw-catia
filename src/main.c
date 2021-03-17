#include <gtk/gtk.h>
#include <pipewire/pipewire.h>

struct data {
    struct pw_thread_loop *loop;
    struct pw_context *context;

    struct pw_core *core;
    struct spa_hook core_listener;

    struct pw_registry *registry;
    struct spa_hook registry_listener;

    struct spa_list globals;
};

struct global {
    uint32_t id;
    struct pw_proxy *proxy;
    const struct spa_dist *props;
    struct spa_list link;
    struct spa_hook proxy_listener;
};

static void handle_proxy_event_removed(void *user_data) {
    struct global *g = user_data;
    printf("[removed] object: id:%u\n", g->id);
    pw_proxy_destroy(g->proxy);
}

static const struct pw_proxy_events proxy_events = {
    PW_VERSION_PROXY_EVENTS,
    .removed = handle_proxy_event_removed,
    // .destroy = destroy_proxy,
};

static void handle_registry_event_global(void *data,
                                         uint32_t id,
                                         uint32_t permissions,
                                         const char *type,
                                         uint32_t version,
                                         const struct spa_dict *props) {
    struct data *d = data;
    struct global *g;
    uint32_t client_version;
    struct pw_proxy *proxy;

    if (strcmp(type, PW_TYPE_INTERFACE_Node) == 0) {
        client_version = PW_VERSION_NODE;
        printf("object: id:%u type:%s/%d name:%s\n",
               id,
               type,
               version,
               spa_dict_lookup(props, PW_KEY_NODE_NAME));
    } else if (strcmp(type, PW_TYPE_INTERFACE_Port) == 0) {
        client_version = PW_VERSION_PORT;
        printf("object: id:%u type:%s/%d name:%s\n",
               id,
               type,
               version,
               spa_dict_lookup(props, PW_KEY_PORT_NAME));
    } else if (strcmp(type, PW_TYPE_INTERFACE_Link) == 0) {
        client_version = PW_VERSION_LINK;
        printf("object: id:%u type:%s/%d input_port:%s output_port:%s\n",
               id,
               type,
               version,
               spa_dict_lookup(props, PW_KEY_LINK_INPUT_PORT),
               spa_dict_lookup(props, PW_KEY_LINK_INPUT_PORT));
    } else {
        return;
    }

    proxy =
        pw_registry_bind(d->registry, id, type, client_version, sizeof(struct global));
    if (proxy == NULL)
        return;

    g = pw_proxy_get_user_data(proxy);
    g->id = id;
    g->proxy = proxy;

    pw_proxy_add_listener(proxy, &g->proxy_listener, &proxy_events, g);
    spa_list_insert(&d->globals, &g->link);
}

static const struct pw_registry_events registry_events = {
    PW_VERSION_REGISTRY_EVENTS,
    .global = handle_registry_event_global,

};

static void activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *window;
    GtkWidget *button;

    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Window");
    gtk_window_set_default_size(GTK_WINDOW(window), 200, 200);

    button = gtk_button_new_with_label("pw-catia");
    gtk_window_set_child(GTK_WINDOW(window), button);

    gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char **argv) {

    struct data data = {0};

    GtkApplication *app;
    int status;

    pw_init(&argc, &argv);
    spa_list_init(&data.globals);

    data.loop = pw_thread_loop_new("thread", NULL /* properties */);
    data.context = pw_context_new(pw_thread_loop_get_loop(data.loop),
                                  NULL /* properties */,
                                  0 /* user_data size */);
    data.core =
        pw_context_connect(data.context, NULL /* properties */, 0 /* user_data size */);
    data.registry =
        pw_core_get_registry(data.core, PW_VERSION_REGISTRY, 0 /* user_data size */);

    spa_zero(data.registry_listener);
    pw_registry_add_listener(data.registry,
                             &data.registry_listener,
                             &registry_events,
                             &data);

    pw_thread_loop_start(data.loop);

    app = gtk_application_new("org.gtk.example", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(activate), &data);

    status = g_application_run(G_APPLICATION(app), argc, argv);

    printf("\nleave_app_loop\n\n");

    pw_thread_loop_stop(data.loop);
    pw_proxy_destroy((struct pw_proxy *)data.registry);
    pw_core_disconnect(data.core);
    pw_context_destroy(data.context);
    pw_thread_loop_destroy(data.loop);
    pw_deinit();

    g_object_unref(app);

    return status;
}
