testmake: main.c
	gcc main.c ui.c dir.c run.c -lncursesw
