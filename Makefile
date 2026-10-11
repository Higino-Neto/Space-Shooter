main: main.c camera.c camera.h player.c player.h planet.c planet.h noise.c noise.h projectile.c projectile.h world.c world.h enemy.c enemy.h skybox.c skybox.h trail.c trail.h wave.c wave.h ui/ui.c ui/ui.h
	gcc main.c camera.c player.c planet.c noise.c projectile.c world.c enemy.c skybox.c trail.c wave.c ui/ui.c -o main $(shell pkg-config --cflags --libs raylib) -lm

run: main
	./main