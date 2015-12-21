//Includes
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <termios.h>
#include <fcntl.h>

//Defines
#define CLEAR "\033[2J"
#define CLEAR_SIZE 7
#define MILLION 1000000

//Typedefs
typedef unsigned short color_t;

//Method Prototypes
void clear_screen();
void exit_graphics();
void init_graphics();
char getkey();
void sleep_ms(long ms);
void draw_pixel(int x, int y, color_t color);
void draw_rect(int x1, int y1, int width, int height, color_t color);
void draw_circle(int x, int y, int r, color_t color);

//Globals
int framebuffer_file;
int framebuffer_size;
int line_length;
short* framebuffer;


void init_graphics(){

	struct fb_var_screeninfo var_screeninfo;
	struct fb_fix_screeninfo fix_screeninfo;
	struct termios terminal_settings;

	//Open Framebuffer Device
	framebuffer_file = open("/dev/fb0", O_RDWR);
	
	if(framebuffer_file >= 0){
	
	//Get Framebuffer Info
	ioctl(framebuffer_file, FBIOGET_VSCREENINFO, &var_screeninfo);
	ioctl(framebuffer_file, FBIOGET_FSCREENINFO, &fix_screeninfo);
	
	line_length = fix_screeninfo.line_length;
	
	framebuffer_size = (var_screeninfo.yres_virtual * fix_screeninfo.line_length);
	
	//Map Framebuffer
	framebuffer = mmap(NULL, framebuffer_size, PROT_READ|PROT_WRITE, MAP_SHARED, framebuffer_file, 0);
	
	
	//Remove Terminal Keypress Echo and Buffering
	ioctl(1, TCGETS, &terminal_settings);
	
	terminal_settings.c_lflag &= ~ECHO;
	terminal_settings.c_lflag &= ~ICANON;
		
	ioctl(1, TCSETS, &terminal_settings);
	
	}
	
	else{
		char error[] = "error opening framebuffer\n";
		write(2, error, sizeof(error)-1); 
    }

}

void exit_graphics(){
	
	struct termios terminal_settings;
	
	//Unmap Memory for Framebuffer
	if(munmap(framebuffer, framebuffer_size) < 0){
		char error[] = "error unmapping framebuffer\n";
		write(2, error, sizeof(error)-1); 
	}

	//Reset Terminal Keypress Echo and Buffering
	ioctl (1, TCGETS, &terminal_settings);
	
	terminal_settings.c_lflag |= ECHO;
	terminal_settings.c_lflag |= ICANON;
		
	ioctl (1, TCSETS, &terminal_settings);
	
	//Close Framebuffer File Descriptor
	if(close(framebuffer_file) < 0){
		char error[] = "error closing framebuffer\n";
		write(2, error, sizeof(error)-1); 
		
	}
}

void clear_screen(){
	
	//Clear the terminal screen using Escape Code
	if (write(1, CLEAR, CLEAR_SIZE) != CLEAR_SIZE){
		char error[]= "error clearing terminal\n";
		write(2, error, sizeof(error)-1); 
	}
}

char getkey(){
	fd_set read_set;
	struct timeval timer;
	int result;
	char c;
	
	//Wait for input for 30 seconds
	timer.tv_sec = 30;
	timer.tv_usec = 0;
	
	//Set read_set to read from StdIn (0)
	FD_ZERO(&read_set);
	FD_SET(0, &read_set);
	
	//Watch StdIn for a keypress
	result = select(1, &read_set, NULL, NULL, &timer);
	
	if(result){
	
		//Read Keypress Char
		if(read(0,&c,1) < 0){
			char error[] = "error reading character\n";
			write(2, error, sizeof(error)-1); 
			c = '\0';
		}
	}
	
	else if(result == 0){
		char error[] = "error: select timed out\n";
		write(2, error, sizeof(error)-1);
		c = '\0';
	}
	
	else{
		char error[] = "error getting key\n";
		write(2, error, sizeof(error)-1);
		c = '\0';
	}
	
	return c;

}

void sleep_ms(long ms){
	struct timespec timer;
	
	//Convert milliseconds to nanoseconds
	timer.tv_nsec = (ms * MILLION);
	timer.tv_sec = 0;
	
	if(nanosleep(&timer,NULL) < 0){
		char error[] = "error clearing terminal\n";
		write(2, error, sizeof(error)-1);
		
	}
	
}

void draw_pixel(int x, int y, color_t color){
	
	short* temp_framebuffer;
	int offset = x + (y*line_length/2);
	
	temp_framebuffer = framebuffer + offset;
	
	*temp_framebuffer = color;

}

void draw_rect(int x1, int y1, int width, int height, color_t color){

	int i;

	for(i = 0; i < width; i++){
		draw_pixel(x1+i, y1, color);
		draw_pixel(x1+i, y1+height, color);
	}
	
	for(i = 0; i < height; i++){
		draw_pixel(x1, y1+i, color);
		draw_pixel(x1+width, y1+i, color);
	}
	
}

void draw_circle(int x1, int y1, int r, color_t color){

  int x = r;
  int y = 0;
  int radiusError = 1-x;
 
  while(x >= y)
  {
    draw_pixel(x + x1, y + y1, color);
    draw_pixel(y + x1, x + y1, color);
    draw_pixel(-x + x1, y + y1, color);
    draw_pixel(-y + x1, x + y1, color);
    draw_pixel(-x + x1, -y + y1, color);
    draw_pixel(-y + x1, -x + y1, color);
    draw_pixel(x + x1, -y + y1, color);
    draw_pixel(y + x1, -x + y1, color);
    y++;
    if (radiusError<0)
    {
      radiusError += 2 * y + 1;
    }
    else
    {
      x--;
      radiusError += 2 * (y - x + 1);
    }
  }

}
















