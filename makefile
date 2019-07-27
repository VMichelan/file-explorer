testmake: main.c
	gcc main.c dir.c run.c -lncursesw -lmagic
