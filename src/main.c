#include <gtk/gtk.h>
#include <pipewire/pipewire.h>

static void registry_event_global(void *data,
                                  uint32_t id,
                                  uint32_t permissions,
                                  const char *type,
                                  uint32_t version,
                                  const struct spa_dict *props) {
    printf("object: id:%u type:%s/%d\n", id, type, version);
}

static const struct pw_registry_events registry_events = {
    PW_VERSION_REGISTRY_EVENTS,
    .global = registry_event_global,
};

static void print_hello(GtkWidget *widget, gpointer data) {
    g_print("Hello World\n");
}

static void activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *window;
    GtkWidget *button;

    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Window");
    gtk_window_set_default_size(GTK_WINDOW(window), 200, 200);

    button = gtk_button_new_with_label("Hello World");
    g_signal_connect(button, "clicked", G_CALLBACK(print_hello), NULL);
    gtk_window_set_child(GTK_WINDOW(window), button);

    gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char **argv) {

    struct pw_thread_loop *loop;
    struct pw_context *context;
    struct pw_core *core;
    struct pw_registry *registry;
    struct spa_hook registry_listener;

    GtkApplication *app;
    int status;

    pw_init(&argc, &argv);

    loop = pw_thread_loop_new("thread", NULL /* properties */);
    context = pw_context_new(pw_thread_loop_get_loop(loop),
                             NULL /* properties */,
                             0 /* user_data size */);
    core = pw_context_connect(context, NULL /* properties */, 0 /* user_data size */);
    registry = pw_core_get_registry(core, PW_VERSION_REGISTRY, 0 /* user_data size */);

    spa_zero(registry_listener);
    pw_registry_add_listener(registry, &registry_listener, &registry_events, NULL);

    pw_thread_loop_start(loop);

    app = gtk_application_new("org.gtk.example", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);

    status = g_application_run(G_APPLICATION(app), argc, argv);

    printf("\nleave_app_loop\n");

    pw_thread_loop_stop(loop);
    pw_proxy_destroy((struct pw_proxy *)registry);
    pw_core_disconnect(core);
    pw_context_destroy(context);
    pw_thread_loop_destroy(loop);

    g_object_unref(app);

    return status;
}
