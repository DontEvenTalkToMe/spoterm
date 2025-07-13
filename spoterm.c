#include <stdio.h>
#include <ncurses.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

// Prototypes for functions
void toggleSong();
void getMetadata();
void positionChanger (int direction);
void volumeChanger (int magnitude);   
void queueChanger (int way);
void pointLength();

// Declaration of variables
char status[256];
char prevTitle[32];
char length[256]; 
char title[32];
char album[32]; 
char artist[32]; 

int main (int argAmount, char *argS[]) {
    // Declaring local variables
    int input;
    int sInput;
    int ty, tx, my, mx, cy, cx;
    int delayLength = 3;
    int splashText = 0;
    char plauseButton[] = "[|>/||]";

    // Looking for arguments
    int showPiano = 0;
	char usePlayers[] = " --players=spotify";

    for (int i = 1; i < argAmount; i++) {
        if (strcmp(argS[i], "n") == 0) {
            showPiano = 1; 
        }
    }

    // Setting screen settings 
    initscr();
    noecho();
    getmaxyx(stdscr, ty, tx);
    keypad(stdscr, TRUE);
    curs_set(0);
    timeout(0);	

	// Initialization of content window
	WINDOW * content = newwin(3, tx, (ty / 2), 0);
	getmaxyx(content, cy, cx);
	refresh();
	wrefresh(content);

    // Initialization of menu window
    WINDOW * menu = newwin(3, tx, (ty / 2) + 5, 0);
    box(menu, 0, 0);
    getmaxyx(menu, my, mx);
	wtimeout(menu, 0);
	keypad(menu, TRUE);
    refresh();
    wrefresh(menu);
	mousemask(ALL_MOUSE_EVENTS, NULL);

    mvwprintw(menu, my / 2, (mx - strlen(plauseButton)) / 2, plauseButton);
    printw("Press F1 to quit.");

    int asciiY = (ty / 2) - 10;
    int asciiX = (tx - 42) / 2;

    // Check if user's terminal has colours
    if (has_colors()) {
        start_color();
        use_default_colors();
    }
    else {
        int hi;
        printf("Your terminal does not support colours. Press [Enter] to continue: ");
        scanf("%d", hi);
    }

    // Draw piano as long as -n flag has not been set
    if (argAmount < 2 || showPiano != 1) {
        mvprintw(asciiY + 2, asciiX, "_______________________________________");
        mvprintw(asciiY + 3, asciiX, "|\\                                    \\");
        mvprintw(asciiY + 4, asciiX, "| \\                                    \\");
        mvprintw(asciiY + 5, asciiX, "|  \\____________________________________\\");
        mvprintw(asciiY + 6, asciiX, "|  |       __---_ _---__                |");
        mvprintw(asciiY + 7, asciiX, "|  |      |======|=====|                |");
        mvprintw(asciiY + 8, asciiX, "|  |      |======|=====|                |");
        mvprintw(asciiY + 9, asciiX, "__________________________________________");
        mvprintw(asciiY + 13, asciiX, "__________________________________________");
    }

	MEVENT mouseAction;

	wmove(menu, 0, 0);
    while (true){
        getMetadata();
        if (strcmp(title, prevTitle) != 0) {
            int titleXLocation = ((cx-strlen(title)-strlen(artist)-3) / 2);
            int artistXLocation =  titleXLocation + (strlen(title));

            // Clear screen, remove remnants
		   	wclear(content); 

            // Draw title
            init_pair(1, COLOR_MAGENTA, -1);
            wattron(content, A_UNDERLINE | COLOR_PAIR(1));
            mvwprintw(content, 0, titleXLocation, title);
            wattroff(content, A_UNDERLINE | COLOR_PAIR(1));
            
            // Draw artist
            if (strcmp(artist, "") != 0) {
                mvwprintw(content, 0, artistXLocation, " by %s", artist);
            }
            // Draw album or Single
            if (strcmp(title, album) != 0 && strcmp(album, "") != 0) {
                mvwprintw(content, 1, ((cx - strlen(album) - 7) / 2), "Album: %s", album);
            }
            else {
                mvwprintw(content, 1, (cx - 6) / 2, "Single");
            }
        }
        switch (delayLength) {
            case 3:
                pointLength();
				wmove(content, 2, 0);
				wclrtoeol(content);
				wmove(menu, 0, 0);
                mvwprintw(content, 2, (cx - strlen(length)) / 2, "%s", length);
                delayLength = 0;
                break;    
        }
        refresh();
        wrefresh(menu);
		wrefresh(content);
        input = wgetch(menu);
        switch (input) {
            case ' ':
            case KEY_DC:
                toggleSong();
                break;
            case KEY_F(1):
                endwin();
                exit(0);
                break;
            case KEY_LEFT:
                wtimeout(menu, 300);
                sInput = wgetch(menu);
                if (sInput == KEY_LEFT) {
                    queueChanger(0);
                }
                else {
                    positionChanger(0);
                }
                wtimeout(menu, 0);
                break;
            case KEY_RIGHT:
                wtimeout(menu, 300);
                sInput = wgetch(menu);
                if (sInput == KEY_RIGHT) {
                    queueChanger(1);
                }
                else {
                    positionChanger(1);
                }
                wtimeout(menu, 0);
                break;
            case KEY_DOWN:
                volumeChanger(0);
                break;
            case KEY_UP:
                volumeChanger(1);
                break;
            case 'm':
                volumeChanger(2);
                break;
            case 'n':
                volumeChanger(3);
				break;
			case KEY_MOUSE:
				getmouse(&mouseAction);
				if (mouseAction.bstate & BUTTON1_CLICKED) {
						toggleSong();
				}
				else if (mouseAction.bstate & BUTTON1_DOUBLE_CLICKED) {
						queueChanger(1);
				}
				else if (mouseAction.bstate & BUTTON3_DOUBLE_CLICKED) {
						queueChanger(0);
				}
				break;
        }
        usleep(800000);
        strcpy(prevTitle, title);
        delayLength++;
    }
}

void getMetadata() {
    FILE * mtitle = popen("playerctl --player=spotify,%any metadata title 2>/dev/null", "r");
    // Title
    if (fgets(title, sizeof(title), mtitle) != NULL) {
        title[strcspn(title, "\n")] = 0;
    } 
    else {
        endwin();
        printf("No mpris audio client found not running. Exiting.\n");
        exit(1);
    }
    // Artist and Album
    if (strcmp(prevTitle, title) != 0) {
        // Clering old artist and album
        strcpy(artist, "");
        strcpy(album, "");

        // Getting artist and album
        FILE * martist = popen("playerctl metadata artist 2>/dev/null", "r");
        FILE * malbum = popen("playerctl --player=spotify,%any metadata album 2>/dev/null", "r");
        if (fgets(artist, sizeof(artist), martist) != NULL) {
            artist[strcspn(artist, "\n")] = 0;
        }
        if (fgets(album, sizeof(album), malbum) != NULL) {
            album[strcspn(album, "\n")] = 0;
        } 
        pclose(martist);
        pclose(malbum);
    }
    pclose(mtitle);
}

void toggleSong() {
    FILE * mstatus = popen("playerctl --player=spotify,%any play-pause 2>/dev/null", "r");
    pclose(mstatus);
}

void positionChanger (int direction) {
    FILE * change;
    if (direction == 0) {
        change = popen("playerctl --player=spotify,%any position 10- 2>/dev/null", "r");
    }
    else if (direction == 1) {
        change = popen("playerctl --player=spotify,%any position 10+ 2>/dev/null", "r");
    }
    pclose(change);
}

void volumeChanger (int magnitude) {
    FILE * volume;
    switch (magnitude) {
        case 0:
            volume = popen("playerctl --player=spotify,%any volume 0.1- 2>/dev/null", "r");
            break;
        case 1:
            volume = popen("playerctl --player=spotify,%any volume 0.1+ 2>/dev/null", "r");
            break;
        case 2:
            volume = popen("playerctl --player=spotify,%any volume 1- 2>/dev/null", "r");
            break;
        case 3:
            volume = popen("playerctl --player=spotify,%any volume 1+ 2>/dev/null", "r");
            break;
    }
    pclose(volume);
}

void queueChanger (int way) {
    FILE * queue;
    if (way == 0) {
        queue =  popen("playerctl --player=spotify,%any previous 2>/dev/null", "r");
    }
    else if (way == 1) {
        queue =  popen("playerctl --player=spotify,%any next 2>/dev/null", "r");
    }
}

void pointLength () {
    FILE * mlength = popen("playerctl metadata --player=spotify,%any --format '[{{ duration(position) }}/{{ duration(mpris:length) }}] - {{ status }}'", "r");
    // Length
    if (fgets(length, sizeof(length), mlength) != NULL) {
        length[strcspn(length, "\n")] = 0;
    } 
    pclose(mlength);

}



