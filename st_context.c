#include <png.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



#define THRESHOLD 128

typedef struct {
    unsigned char r, g, b;
} RGB;

RGB** load_image(const char* filename, int* width, int* height) {
    FILE* fp = fopen(filename, "rb");
    if (!fp) {
        fprintf(stderr, "Failed to open file %s\n", filename);
        return NULL;
    }

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) {
        fprintf(stderr, "Failed to create png read struct\n");
        fclose(fp);
        return NULL;
    }

    png_infop info = png_create_info_struct(png);
    if (!info) {
        fprintf(stderr, "Failed to create png info struct\n");
        png_destroy_read_struct(&png, NULL, NULL);
        fclose(fp);
        return NULL;
    }

    if (setjmp(png_jmpbuf(png))) {
        fprintf(stderr, "Error reading png file\n");
        png_destroy_read_struct(&png, &info, NULL);
        fclose(fp);
        return NULL;
    }

    png_init_io(png, fp);
    png_read_info(png, info);

    *width = png_get_image_width(png, info);
    *height = png_get_image_height(png, info);

    RGB** image = (RGB**)malloc(*height * sizeof(RGB*));
    for (int i = 0; i < *height; i++) {
        image[i] = (RGB*)malloc(*width * sizeof(RGB));
    }

    png_bytepp rows = (png_bytepp)malloc(*height * sizeof(png_bytep));
    for (int i = 0; i < *height; i++) {
        rows[i] = (png_bytep)malloc(png_get_rowbytes(png, info));
    }

    png_read_image(png, rows);

    for (int i = 0; i < *height; i++) {
        for (int j = 0; j < *width; j++) {
            image[i][j].r = rows[i][j * 3];
            image[i][j].g = rows[i][j * 3 + 1];
            image[i][j].b = rows[i][j * 3 + 2];
        }
    }

    for (int i = 0; i < *height; i++) {
        free(rows[i]);
    }
    free(rows);

    png_destroy_read_struct(&png, &info, NULL);
    fclose(fp);

    return image;
}

void save_image(const char* filename, RGB** image, int width, int height) {
    FILE* fp = fopen(filename, "wb");
    if (!fp) {
        fprintf(stderr, "Failed to open file %s\n", filename);
        return;
    }

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) {
        fprintf(stderr, "Failed to create png write struct\n");
        fclose(fp);
        return;
    }

    png_infop info = png_create_info_struct(png);
    if (!info) {
        fprintf(stderr, "Failed to create png info struct\n");
        png_destroy_write_struct(&png, NULL);
        fclose(fp);
        return;
    }

    if (setjmp(png_jmpbuf(png))) {
        fprintf(stderr, "Error writing png file\n");
        png_destroy_write_struct(&png, &info);
        fclose(fp);
        return;
    }

    png_init_io(png, fp);

    png_set_IHDR(
        png,
        info,
        width,
        height,
        8,
        PNG_COLOR_TYPE_RGB,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_DEFAULT,
        PNG_FILTER_TYPE_DEFAULT
    );

    png_write_info(png, info);

    png_bytepp rows = (png_bytepp)malloc(height * sizeof(png_bytep));
    for (int i = 0; i < height; i++) {
        rows[i] = (png_bytep)malloc(3 * width * sizeof(png_byte));
        for (int j = 0; j < width; j++) {
            rows[i][j * 3] = image[i][j].r;
            rows[i][j * 3 + 1] = image[i][j].g;
            rows[i][j * 3 + 2] = image[i][j].b;
        }
    }

    png_write_image(png, rows);
    png_write_end(png, NULL);

    for (int i = 0; i < height; i++) {
        free(rows[i]);
    }
    free(rows);

    png_destroy_write_struct(&png, &info);
    fclose(fp);
}

RGB** convert_to_monochrome(RGB** image, int width, int height) {
    RGB** new_image = (RGB**)malloc(height * sizeof(RGB*));
    for (int i = 0; i < height; i++) {
        new_image[i] = (RGB*)malloc(width * sizeof(RGB));
        for (int j = 0; j < width; j++) {
            unsigned char gray = (image[i][j].r + image[i][j].g + image[i][j].b) / 3;
            new_image[i][j].r = new_image[i][j].g = new_image[i][j].b = gray > THRESHOLD ? 255 : 0;
        }
    }
    return new_image;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s FILENAME\n", argv[0]);
        return 1;
    }

    const char* filename = argv[1];

    int width, height;
    RGB** image = load_image(filename, &width, &height);
    if (!image) {
        fprintf(stderr, "Failed to load image\n");
        return 1;
    }
    RGB** new_image = convert_to_monochrome(image, width, height);
    save_image(new_image, width, height);
    return 0;
}
