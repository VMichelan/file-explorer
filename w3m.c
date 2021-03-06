#define W3MIMGDISPLAYPATH "/usr/lib/w3m/w3mimgdisplay"

struct w3m_config {
    int pid;
    FILE *fout;
    FILE *fin;
    int max_width_pixels;
    int max_height_pixels;
    int fontx;
    int fonty;
    int current_img_width;
    int current_img_height;
    int startx, starty;
};

struct w3m_config w3m_config;

int w3m_set_dimensions(int begx, int begy, int maxx, int maxy) {
    struct winsize screen_size;

    screen_size.ws_col = 0;
    screen_size.ws_row = 0;

    ioctl(0, TIOCGWINSZ, &screen_size);

    if (!screen_size.ws_col || !screen_size.ws_row) {
        return -1;
    }

    int max_width_pixels, max_height_pixels;
    int fontx = screen_size.ws_xpixel / screen_size.ws_col;
    int fonty = screen_size.ws_ypixel / screen_size.ws_row;

    w3m_config.fontx = fontx;
    w3m_config.fonty = fonty;
    w3m_config.max_width_pixels = maxx * fontx;
    w3m_config.max_height_pixels = maxy * fonty;

    int startx, starty;
    w3m_config.startx = (begx * w3m_config.fontx);
    w3m_config.starty = (begy * w3m_config.fonty);

    return 0;
}

void w3m_start(int begx, int begy, int maxx, int maxy) {
    int inpipe[2], outpipe[2];

    pipe(inpipe);
    pipe(outpipe);

    int status;
    int pid = fork();

    if (pid == 0) {
        close(outpipe[1]);
        close(inpipe[0]);
        dup2(outpipe[0], STDIN_FILENO);
        dup2(inpipe[1], STDOUT_FILENO);
        dup2(inpipe[1], STDERR_FILENO);
        execl(W3MIMGDISPLAYPATH, "w3mimgdisplay", (char *) NULL);
        exit(1);
    }

    close(outpipe[0]);
    close(inpipe[1]);

    FILE *fin, *fout;
    fout = fdopen(outpipe[1], "w");
    fin = fdopen(inpipe[0], "r");

    w3m_config.pid = pid;
    w3m_config.fout = fout;
    w3m_config.fin = fin;
}

void w3m_kill() {
    if (!w3m_config.pid)
        return;
    fclose(w3m_config.fout);
    fclose(w3m_config.fin);

    kill(w3m_config.pid, SIGTERM);
    int status;
    waitpid(w3m_config.pid, &status, 0);

    w3m_config.pid = 0;
}

int w3m_get_img_info(char *path, char *file_name, int *width, int *height) {
    char buffer[1024];
    int img_width, img_height;
    fprintf(w3m_config.fout, "5;%s/%s\n", path, file_name);
    fflush(w3m_config.fout);
    fgets(buffer, 1024, w3m_config.fin);
    if (sscanf(buffer, "%d %d", &img_width, &img_height) == 0) {
        return 0;
    }

    if (img_width > w3m_config.max_width_pixels) {
        img_height = (img_height * w3m_config.max_width_pixels) / img_width;
        img_width = w3m_config.max_width_pixels;
    }
    if (img_height > w3m_config.max_height_pixels) {
        img_width = (img_width * w3m_config.max_height_pixels) / img_height;
        img_height = w3m_config.max_height_pixels;
    }

    *width = img_width;
    *height = img_height;

    return 1;
}

void w3m_preview_image(char *path, char* file_name) {
    int img_width, img_height;
    if (w3m_get_img_info(path, file_name, &img_width, &img_height)) {
        fprintf(w3m_config.fout, "0;1;%d;%d;%d;%d;;;;;%s/%s\n4;\n3;\n", w3m_config.startx, w3m_config.starty, img_width, img_height, path, file_name);
        fflush(w3m_config.fout);
    }
    w3m_config.current_img_width = img_width;
    w3m_config.current_img_height = img_height;
}

void w3m_clear(int begx, int begy) {
    char buffer[1024];
    fgets(buffer, 1024, w3m_config.fin);

    fprintf(w3m_config.fout, "6;%d;%d;%d;%d;\n4;\n3;\n", w3m_config.startx, w3m_config.starty, w3m_config.current_img_width + w3m_config.fontx, w3m_config.current_img_height + w3m_config.fonty);
    fflush(w3m_config.fout);
    fgets(buffer, 1024, w3m_config.fin);
}
