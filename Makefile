CFLAGS ?= -O2 -pipe

main: main.c camera.c camera.h player.c player.h planet.c planet.h noise.c noise.h projectile.c projectile.h world.c world.h enemy.c enemy.h skybox.c skybox.h trail.c trail.h wave.c wave.h ui/ui.c ui/ui.h upgrade.c upgrade.h ui/levelup.c ui/levelup.h ui/crosshair.c ui/crosshair.h ui/targeting.c ui/targeting.h
	gcc $(CFLAGS) main.c camera.c player.c planet.c noise.c projectile.c world.c enemy.c skybox.c trail.c wave.c ui/ui.c upgrade.c ui/levelup.c ui/crosshair.c ui/targeting.c -o main $(shell pkg-config --cflags --libs raylib) -lm

run: main
	./main
