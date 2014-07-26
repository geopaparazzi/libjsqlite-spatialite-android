#include <stdlib.h>
#include <stdio.h>

#include "rasterlite2/rasterlite2.h" 

static void
test_rgb_jpeg(const char *path)
{
	rl2SectionPtr img = rl2_section_from_jpeg(path);
fprintf(stderr, "read=%d\n", img);
	if (img == NULL)
		return;

	if (rl2_section_to_png(img, "./dsc_1470.png") != RL2_OK)
fprintf(stderr, "png-out-error\n");

	rl2_destroy_section(img);
}

static void
test_gray_jpeg(const char *path)
{
	rl2SectionPtr img = rl2_section_from_jpeg(path);
fprintf(stderr, "read=%d\n", img);
	if (img == NULL)
		return;

	if (rl2_section_to_png(img, "./dsc_1470gray.png") != RL2_OK)
fprintf(stderr, "png-out-error\n");

	rl2_destroy_section(img);
}

static void
test_palette_png(const char *path)
{
	rl2SectionPtr img = rl2_section_from_png(path);
fprintf(stderr, "read=%d\n", img);
	if (img == NULL)
		return;

	if (rl2_section_to_jpeg(img, "./fig-8.jpg", 20) != RL2_OK)
fprintf(stderr, "jpg-out-error\n");

	rl2_destroy_section(img);
}

static void
test_gray_png(const char *path)
{
	rl2SectionPtr img = rl2_section_from_png(path);
fprintf(stderr, "read=%d\n", img);
	if (img == NULL)
		return;

	if (rl2_section_to_jpeg(img, "./dsc_1470gr16.jpg", 70) != RL2_OK)
fprintf(stderr, "jpg-out-error\n");

	rl2_destroy_section(img);
}

int main(int argc, char *argv[])
{
	test_rgb_jpeg("./DSC_1470.JPG");
	test_gray_jpeg("./DSC_1470gray.jpg");
	test_palette_png("./fig-8.png");
	return 0;
}
