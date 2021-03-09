project('pw-catia', 'c')

src = ['src/main.c']
incdir = include_directories('/usr/include/pipewire-0.3', '/usr/include/spa-0.2')
pipewire = dependency('libpipewire-0.3', version : '>=0.3.23')
gtk = dependency('gtk4', version : '>=4.0')

executable('pw-catia', src, include_directories : incdir, dependencies : [pipewire, gtk]) 
