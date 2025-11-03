#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "time.h"

#include "esat/window.h"
#include "esat/draw.h"
#include "esat/input.h"
#include "esat/sprite.h"
#include "esat/time.h"

#include "esat_extra/sqlite3.h"

#define WIN32 //Sound can not be played on other OS (At least MacOS), so WIN32 is defined when program is played on Windows10
//If program is used on other OS than Windows10 -> Comment this section

#ifdef WIN32
	#include "esat_extra/soloud/soloud.h"
	#include "esat_extra/soloud/soloud_wav.h"
	#undef DrawText
#endif

#ifdef WIN32
	SoLoud::Soloud soloud;
	SoLoud::Wav audio[2];
#endif

#define LinesInScroll 8

const int kWindowWidth = 1250, kWindowHeight = 700;
float scroll_offset_x;
int InfoScroll = 0;
int TableScroll = 0;
int TableCount = 0;
char CurrentTable[50];
const int NumbOfLines = 12;


char query_results[256][256];  // Stores up to 256 lines of query results
int total_results = 0;         // Number of lines currently stored
int display_offset = 0;        // Index of the first line currently displayed
int MouseY = 0;

struct Vec2{	//Struct to use points of references and vectors
	float x,y;
};

//Struct that controlls every aspect of terminal query and input to the bbdd
struct Console{
	Vec2 points[4]; //Points of the rectangular window

	Vec2 windowview[4];	//Points to rectangle that gets name of that window
	Vec2 infotext;	//Coords of text that displays on top
	char infostring[50];	//String that displays text on top

	Vec2 terminal[10];	//Points to draw every aspect of the terminal / query -> rectangle for text && rectangle for button && writing line
	Vec2 terminal_text;	//Were to display rectangle for terminal
	char terminal_string[125];//String that contains submission

	Vec2 enter_text;		//Coords of button text
	char enter_string[50];		//String that displays button text

	
	Vec2 result_text;		//Coords of result of query submission
	char result_string[NumbOfLines][256];	//Every string that contains historial of query

	Vec2 triangle[3];

	char font[70];
	int fontsize = 15;
};
//Struct that controls every aspect of the Viewer
struct Viewer{
	Vec2 points[4]; //Points of the rectangular window
	Vec2 windowview[4]; //Points to the rectangle

	Vec2 infotext; //Vec2 to text that displays on it
	char infostring[50]; //String that displays text on it

	Vec2 triangle[3]; //Vec2 of the small triangle
};
//Struct that controls the tables
struct Tables{
	char table_namestring[50]; //Name of the table
	struct Tables *next; //Next on the list
	Vec2 stringpoints; //Vec2 for the name
  	int ColumnCount; //Number of columns
	struct Columns *columns; //List of columns for each table
	struct Info *info; //List of info for each table
	int row_ammount; //Number of rows

};
//Struct that controls the columns
struct Columns{
	char column_namestring[50]; //Name of the column
	struct Columns *next; //Next on the list
  	Vec2 points[4]; //Vec2 for the rectangle
	Vec2 stringpoints; //Vec2 for the name
	char ownertable_namestring[50]; //Name of it's table
};
//Struct that controls the info
struct Info{
	char data_namestring[50]; //Data stored
	struct Info *next; //Next on the list
	Vec2 stringpoints; //Vec2 for the
	float col_index; //Index for drawing
	float row_index; //Index for drawing
};
//Struct that controls the names
struct TableName{
	Vec2 points[4]; //Vec2 for the rectangle
	char name[50]; //Name of the table
	struct TableName *next; //Next on the list
};
//NULLs the lists
struct Tables *tablelist = NULL;
struct TableName *tablenamelist = NULL;

//Functions to Init || Deinit Everything in the Database editor
void InitEsat();	//Init esat window
void InitSound();	//Init sound
void InitWindows(Console** console, Viewer** viewer, TableName** tablename); //Function to Init Every Graphic window

void FreeSound(); //Deinit Sound
void ClosePointers(Console* console, Viewer* viewer);	//Function to close every pointer

void DrawWindows(Console* console, Viewer* viewer); //Function to draw every graphic aspect
void DrawTables(TableName* tablename);
void InfoScroller();

void DrawQueryTextLine(Console* console, bool* iswriting); //Function to draw query line

void DrawResultText(Console* console);	//Function to draw every cue line
void HandleMouseScroll();
void DrawTriangle(bool direction, float posX, float posY, int offsetY);

void SendQuery(Console* console, char* sql);	//Function that sends query to bbdd and gets response
int QueryTexting(Console* console, bool* iswriting,Tables* currenttable,Columns* currentcolumn,TableName* currenttablename,Info* info);	//Function to get text input
static int callback(void *data, int argc, char **argv, char **azColName);	//Callback from bbdd

bool MouseInSquare(Vec2 point0, Vec2 point2);	//Function to calc if mouse is inside a square

void ResetString(char* string, int caracters);	//Function to Reset a String to \0 every letter

void Sql(); //Function for the database calculations
void CheckTable(); //Checks the name of the table selected
static int TableCallBack(void *data,int argc,char **argv,char **azColName);
void FreeTableNameList(struct TableName* currenttable);
void FreeTableList(struct Tables* tbl);
void FreeInfoList(struct Info* info);
void FreeColumnsList(struct Columns* col);

void InitPoints(TableName** tablename);

void ResetTables(Tables* currenttable,Columns* currentcolumn,TableName* currenttablename,Info* info);

int esat::main(int argc, char **argv){

	unsigned char fps = 25; //Frames per second
	int frameCounter = 0;
	double deltaTime = 0.0, current_time = 0.0, last_time = 0.0; //Timers for while bucle
	//Boolean to check if user is writing or not
	bool writing = false;

	//Struct initialization
	Viewer* viewer = NULL;
	Console* console = NULL; 
	TableName* tablename = NULL;
	Tables* currenttable = NULL;
	Columns* currentcolumn = NULL;
	Info* info = NULL;

	Sql();
	InitEsat(); //To init esat window
	InitWindows(&console,&viewer,&tablename);	//To init every struct for the windows
	InitPoints(&tablename);
	InitSound();

	while(esat::WindowIsOpened() && !esat::IsSpecialKeyDown(esat::kSpecialKey_Escape)){
		last_time = esat::Time();
		do{
			current_time = esat::Time();
			deltaTime = current_time - last_time;
		} while((deltaTime) <= 1000.0 / fps);
		//Begin of frame
		frameCounter = (frameCounter>=fps)?0:frameCounter +1;
		

		//QueryTexting gets user writing inputs
		QueryTexting(console, &writing,currenttable,currentcolumn,tablename,info);

		//Drawing functions
		esat::DrawBegin();
		esat::DrawClear(0,0,0);
		esat::DrawSetStrokeColor(255,50,50,80);
		esat::DrawSetFillColor(255,255,255,5);

		DrawWindows(console,viewer);	//Function to draw every window and button displayed

		DrawTables(tablename);

		DrawQueryTextLine(console, &writing);	//Function to draw interactive text line (Query writeable line)

		DrawResultText(console);	//Function to draw text that displays query historial

		esat::DrawEnd();

    CheckTable();
		InfoScroller();
		HandleMouseScroll();
		esat::WindowFrame(); //End of that frame
  }
  esat::WindowDestroy();
	FreeSound();
  ClosePointers(console, viewer); //Function to shut all pointers
  return 0;
    
}

void InitEsat(){
	esat::WindowInit(kWindowWidth,kWindowHeight);
	esat::WindowSetMouseVisibility(true);
}

//Reverses the list
struct Tables* ReverseList(struct Tables* head){
    struct Tables* prev = NULL;
    struct Tables* current = head;
    struct Tables* next = NULL;

    while(current != NULL){
        next = current->next;
        current->next = prev;
        prev = current;
        current = next;
    }
    return prev;
}
//Reverses the list
struct TableName* ReverseNames(struct TableName* head){
    struct TableName* prev = NULL;
    struct TableName* current = head;
    struct TableName* next = NULL;

    while(current != NULL){
        next = current->next;
        current->next = prev;
        prev = current;
        current = next;
    }
    return prev;
}
//Reverses the list
struct Columns* ReverseColumns(struct Columns* head){
    struct Columns* prev = NULL;
    struct Columns* current = head;
    struct Columns* next = NULL;

    while (current != NULL){
        next = current->next;
        current->next = prev;
        prev = current;
        current = next;
    }
    return prev;
}
//Reverses the list
struct Info* ReverseInfo(struct Info* head){
    struct Info* prev = NULL;
    struct Info* current = head;
    struct Info* next = NULL;

    while (current != NULL){
        next = current->next;
        current->next = prev;
        prev = current;
        current = next;
    }
    return prev;
}
//Callback for getting the table names from the db
static int TableCallBack(void *data,int argc,char **argv,char **ColName){
	if(argc > 0 && argv[0] !=NULL){
		struct Tables *currenttable = (struct Tables*)malloc(sizeof(struct Tables));
		//Resets it to 0 to avoid crashes
		memset(currenttable, 0, sizeof(struct Tables));
		strncpy(currenttable->table_namestring,argv[0],49); //Adds the name to the namestring
		currenttable->table_namestring[49] ='\0'; //Makes sure it ends properly
		currenttable->ColumnCount = 0;
		TableCount++; //Counts the tables
		currenttable->next = tablelist;
    	tablelist = currenttable;
	}
	return 0;
}
//Callback for getting the column info from the db
static int ColumnTableCallBack(void *data,int argc,char **argv,char **ColName){
	if(argc > 0 && argv[0] !=NULL){
		struct Tables *currenttable = (struct Tables*)data;
		struct Columns *currentcolumn = (struct Columns*)malloc(sizeof(struct Columns));
		strncpy(currentcolumn->column_namestring,argv[1],49); //Adds the name to the namestring
		currentcolumn->column_namestring[49] ='\0'; //Makes sure it ends properly
		currenttable->ColumnCount++; //Counts the columns
		strcpy(currentcolumn->ownertable_namestring,currenttable->table_namestring); //Adds the name to the namestring
		currentcolumn->next = currenttable->columns;
		currenttable->columns = currentcolumn;
	}
	return 0;
}
//Callback for getting the info from the db
static int InfoCallback(void *data, int argc, char **argv, char **ColName){
    struct Tables *currenttable = (struct Tables*)data;

    for(int i = 0; i < argc; i++){
        struct Info *info = (struct Info*)malloc(sizeof(struct Info));
        memset(info, 0, sizeof(struct Info)); //Resets it to 0 to avoid crashes
        if(argv[i] != NULL){
            strncpy(info->data_namestring, argv[i], 49); //Adds the info to the string
		}
        else{
            strcpy(info->data_namestring, "NULL"); //If it's empty it adds NULL
		}
        info->data_namestring[49] = '\0'; //Makes sure it ends properly

        info->col_index = i; //Column index for drawing
        info->row_index = currenttable->row_ammount; //Row index for drawing

        info->next = currenttable->info;
        currenttable->info = info;
    }

    currenttable->row_ammount++;
    return 0;
}


void Sql(){
	sqlite3* DB;
	//Reads DB
	char file[30];
	snprintf(file, sizeof(char) * 30, "assets/database/database.db");

	int exist = sqlite3_open(file, &DB);

	//If it doesn't exist it gives an error
	if(exist != SQLITE_OK){
		printf("Error opening\n");
	}else{
		//String to get the names of the tables
		char sql[150];
		snprintf(sql, sizeof(char) * 150, "SELECT name FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%%'");

		exist = sqlite3_exec(DB, sql, TableCallBack, 0, 0);
    
		struct Tables *currenttable = tablelist;
		//String to get the info and columns for each table
		while(currenttable != NULL){
			snprintf(sql, sizeof(sql), "PRAGMA table_info(%s);",currenttable->table_namestring);
			exist = sqlite3_exec(DB,sql,ColumnTableCallBack,currenttable,0);
			currenttable->columns = ReverseColumns(currenttable->columns);

			snprintf(sql, sizeof(sql), "SELECT * FROM \"%s\";",currenttable->table_namestring);
			exist = sqlite3_exec(DB,sql,InfoCallback,currenttable,0);
			currenttable->info = ReverseInfo(currenttable->info);

			currenttable = currenttable->next;
		}

		if(exist != SQLITE_OK){
			printf("Error reading\n");
		}
		//Closes the db
		sqlite3_close(DB);
    	tablelist = ReverseList(tablelist);
	}
	
}

//Checks which table is currently active
void CheckTable(){
	struct TableName* currenttablename = tablenamelist;
	struct Tables* currenttable = tablelist;

	while(currenttable != NULL){
		Vec2 curpoint0 = currenttablename->points[0];
		curpoint0.x -= scroll_offset_x;

		Vec2 curpoint2 = currenttablename->points[2];
		curpoint2.x -= scroll_offset_x;

		if(MouseInSquare(curpoint0, curpoint2) && esat::MouseButtonDown(0)){
			strncpy(CurrentTable, currenttable->table_namestring, 49);
			CurrentTable[49] = '\0';
			InfoScroll = 0;
			#ifdef WIN32
					soloud.play(audio[0], 1.0f, 0.0f);
			#endif
			break;
		}
		currenttable = currenttable->next;
		currenttablename = currenttablename->next;
	}
}

//Initialization of every struct and data insertion
void InitWindows(Console** console,Viewer** viewer,TableName** tablename){
	*viewer = (Viewer*) malloc(sizeof(Viewer));
	*console = (Console*) malloc(sizeof(Console));

	//Functions to reset every string displayed
	ResetString((*console)->infostring,50);
	ResetString((*console)->terminal_string,125);
	for(int i = 0; i < NumbOfLines; i++){
		ResetString((*console)->result_string[i], 125);
	}
	ResetString((*console)->enter_string,50);
	//Some coords for later calculation, Ex: Border -> distance between windows
	float border = 5;
	float topviewerborder = 15; //Distance to top window border
	float bottomviewerborder = 5;	//Distance to bottom window border
	float middle = kWindowHeight / 2;	//Hald screen

	//Defines the viewer rectangle coordinates
	(*viewer)->points[0].x = border;
	(*viewer)->points[0].y = topviewerborder;
	
	(*viewer)->points[1].x = kWindowWidth - border;
	(*viewer)->points[1].y = topviewerborder;
	
	(*viewer)->points[2].x = kWindowWidth - border;
	(*viewer)->points[2].y = middle - bottomviewerborder;

	(*viewer)->points[3].x = border;
	(*viewer)->points[3].y = middle - bottomviewerborder;

	(*viewer)->windowview[0].x = (*viewer)->points[0].x;
	(*viewer)->windowview[0].y = (*viewer)->points[0].y;

	(*viewer)->windowview[1].x = (*viewer)->points[1].x;
	(*viewer)->windowview[1].y = (*viewer)->points[1].y;

	(*viewer)->windowview[2].x = (*viewer)->points[1].x;
	(*viewer)->windowview[2].y = (*viewer)->points[1].y + border * 4;

	(*viewer)->windowview[3].x = (*viewer)->points[0].x;
	(*viewer)->windowview[3].y = (*viewer)->points[0].y + border * 4;
	//Coords for the text
	(*viewer)->infotext.x = border + 20;
	(*viewer)->infotext.y = topviewerborder + 15;

	snprintf((*viewer)->infostring, sizeof(char) * 50, "Schema");

	//Viewer Triangle
	(*viewer)->triangle[0].x = border + 7;
	(*viewer)->triangle[0].y = topviewerborder + 8;
	(*viewer)->triangle[1].x = border + 15;
	(*viewer)->triangle[1].y = topviewerborder + 8;
	(*viewer)->triangle[2].x = border + 11;
	(*viewer)->triangle[2].y = topviewerborder + 14;


	//Console Windwow Points, these are the corners of the console window
	(*console)->points[0].x = border;
	(*console)->points[0].y = middle + border;

	(*console)->points[1].x = kWindowWidth - border;
	(*console)->points[1].y = middle + border;

	(*console)->points[2].x = kWindowWidth - border;
	(*console)->points[2].y = kWindowHeight - border;

	(*console)->points[3].x = border;
	(*console)->points[3].y = kWindowHeight - border;

	//Console View -> Console name info && console name rectangle
	(*console)->windowview[0].x = (*console)->points[0].x;
	(*console)->windowview[0].y = (*console)->points[0].y;

	(*console)->windowview[1].x = (*console)->points[1].x;
	(*console)->windowview[1].y = (*console)->points[1].y;
	
	(*console)->windowview[2].x = (*console)->points[1].x;
	(*console)->windowview[2].y = (*console)->points[1].y + border * 4;

	(*console)->windowview[3].x = (*console)->points[0].x;
	(*console)->windowview[3].y = (*console)->points[0].y + border * 4;

	//String that contais info about that window
	snprintf((*console)->infostring, sizeof(char) * 50, "Query");
	
	//Coords of that text
	(*console)->infotext.x = border + 22;
	(*console)->infotext.y = middle + border + 15;

	//Console Triangle
	float dist_down = 339;
	(*console)->triangle[0].x = border + 7;
	(*console)->triangle[0].y = topviewerborder + dist_down + 8;
	(*console)->triangle[1].x = border + 15;
	(*console)->triangle[1].y = topviewerborder + dist_down + 8;
	(*console)->triangle[2].x = border + 11;
	(*console)->triangle[2].y = topviewerborder + dist_down + 14;

	//This part draws rectangle where text can be written (Query Line) && Send Query button
	//Setting terminal border points
	for(int i = 0; i < 8; i++){
		(*console)->terminal[i].x = (*console)->windowview[i % 4].x;
		(*console)->terminal[i].y = (*console)->windowview[i % 4].y + 40;
	}
	//This substracts or sums 10 to make the border inside the window
	int offset[8] ={10, -10, -10, 10, 10, -10, -10, 10};
	for(int i = 0; i < 8; i++){
			(*console)->terminal[i].x += offset[i];
	}
	//This part makes enter button smaller
	(*console)->terminal[5].x -= 1116;
	(*console)->terminal[6].x -= 1116;
	//Here is just for moving enter button down
	for(int i = 4; i < 8; i++){
		(*console)->terminal[i].y += 30;
	}
	//Send Query Button text coords
	(*console)->enter_text.x = border + 13;
	(*console)->enter_text.y = (*console)->terminal[4].y + 15;
	snprintf((*console)->enter_string, sizeof(char) * 50, "Send Query");

	//Terminal Text, Where text can be written
	(*console)->terminal_text.x = border + 15;
	(*console)->terminal_text.y = (*console)->terminal[0].y + 15;
	snprintf((*console)->terminal_string, sizeof(char) * 125, "");

	//This coords set where to display query historial
	(*console)->result_text.x = (*console)->enter_text.x;
	(*console)->result_text.y = (*console)->enter_text.y + 50;

	snprintf((*console)->font, sizeof(char) * 70, "assets/font/VCR_OSD_MONO_1.001.ttf");
	(*console)->fontsize = 15;
}

void InitPoints(TableName** tablename){
	float border = 5;
	float topviewerborder = 15; //Distance to top window border
	float bottomviewerborder = 15;	//Distance to bottom window border
	float middle = kWindowHeight / 2;	//Hald screen

	struct Tables *currenttable = tablelist;
	float currentx = border + 7;

		//Calculates where the x and y are for the info
	while(currenttable != NULL){
		struct Info *info = currenttable->info;
		int row_count = 0;

		while (info != NULL){
			//Calculates the width based on the table's column count and the window width
			float xlength = (kWindowWidth - border) / currenttable->ColumnCount;
			//Calculates x and y
			float x = xlength * info->col_index + 10;
			float y = 15 + 100 + row_count * 20;
			//Adds the positions to the string
			info->stringpoints.x = x + 3;
			info->stringpoints.y = y + 3;

			info = info->next;
			if(info != NULL && info->col_index == 0){
				row_count++; //Counts another row
			}
		}
		currenttable = currenttable->next;
	}
  currenttable = (Tables*) calloc(sizeof(Tables),1);
	currenttable = tablelist;
	//Set positions for the names of the tables
	for(int i=0; i<TableCount;i++){
			struct TableName *tablename = (struct TableName*)malloc(sizeof(struct TableName));
			int text_len = strlen(currenttable->table_namestring);
			//Aprox width of a character
			float charwidth = 9.0f;
			float textwidth = text_len * charwidth + 5.0f;
			//Makes the rectangle for the table name
			tablename->points[0].x = currentx;
			tablename->points[0].y = topviewerborder + 30;
			
			tablename->points[1].x = currentx + textwidth;
			tablename->points[1].y = topviewerborder + 30;
			
			tablename->points[2].x = currentx + textwidth;
			tablename->points[2].y = topviewerborder + 50;
			
			tablename->points[3].x = currentx;
			tablename->points[3].y = topviewerborder + 50;
			//Position for the text
			currenttable->stringpoints.x = tablename->points[0].x + 3;
			currenttable->stringpoints.y = tablename->points[0].y + 15;

			tablename->next = tablenamelist;
			tablenamelist = tablename;

			currenttable = currenttable->next;

			currentx += textwidth + 10;
	}
	//Reverses the list
	tablenamelist = ReverseNames(tablenamelist);

	currenttable = tablelist;

	//Set positions for the columns
	for(int j=0;j<TableCount;j++){
		struct Columns *currentcolumn = currenttable->columns;
		int ColumnCount = 0;

		while(currentcolumn != NULL){
			//Calculates the width based on the table's column count and the window width
			float xlength = (kWindowWidth - border) / currenttable->ColumnCount;
			float letterSize = 10;
			//Defines the four corner points of the rectangle
			currentcolumn->points[0].x = xlength * ColumnCount + letterSize;
			currentcolumn->points[0].y = topviewerborder + 60 ;

			currentcolumn->points[1].x = xlength * ColumnCount;
			currentcolumn->points[1].y = topviewerborder + 60 ;

			currentcolumn->points[2].x = xlength * ColumnCount + letterSize;
			currentcolumn->points[2].y = topviewerborder + 60 ;

			currentcolumn->points[3].x = xlength * ColumnCount;
			currentcolumn->points[3].y = topviewerborder + 60 ;
			//Position for the column header text
			currentcolumn->stringpoints.x = currentcolumn->points[0].x + 3;
			currentcolumn->stringpoints.y = currentcolumn->points[0].y + 15;

			currentcolumn = currentcolumn->next;

			ColumnCount++;

		}
		currenttable = currenttable->next;
	}
}
//Function to draw every window, and every graphic aspect, excluding some strings
void DrawWindows(Console* console,Viewer* viewer){
	
	esat::DrawSetTextFont(console->font);
	esat::DrawSetTextSize(console->fontsize);
	//Console && Viewer Borders and form
	esat::DrawSetStrokeColor(255,50,50,80);
	esat::DrawSetFillColor(255,255,255,5);
	esat::DrawSolidPath(&console->points[0].x, 4);
	esat::DrawSolidPath(&viewer->points[0].x, 4);
	//Console && Viewer info
	esat::DrawSetStrokeColor(255,50,50,80);
	esat::DrawSetFillColor(255,50,50,80);
	esat::DrawSolidPath(&console->windowview[0].x, 4);
	esat::DrawSolidPath(&viewer->windowview[0].x, 4);
	//Console && Viewer Info text
	esat::DrawSetFillColor(15,15,15,255);
  esat::DrawText(console->infotext.x, console->infotext.y, console->infostring);
  esat::DrawText(viewer->infotext.x, viewer->infotext.y, viewer->infostring);

	//Terminal
	esat::DrawSetFillColor(255,50,50,50);
	esat::DrawSetStrokeColor(255,50,50,80);
	esat::DrawSolidPath(&console->terminal[0].x, 4);

	//Terminal Query Text
	esat::DrawSetFillColor(255,255,255,255);
  esat::DrawText((console)->terminal_text.x, (console)->terminal_text.y, (console)->terminal_string);

	//Terminal Enter button
	if(MouseInSquare(console->terminal[4],console->terminal[6])){
		esat::DrawSetFillColor(255,50,50,80);
	}else{
		esat::DrawSetFillColor(255,50,50,120);
	}
	esat::DrawSetStrokeColor(255,50,50,80);
	esat::DrawSolidPath(&console->terminal[4].x, 4);

	//Terminal Enter Button text
	esat::DrawSetFillColor(255,255,255,255);
  esat::DrawText((console)->enter_text.x, (console)->enter_text.y, (console)->enter_string);

  //Viewer && Console Triangle
	esat::DrawSetStrokeColor(255,255,255,255);
	esat::DrawSetFillColor(255,255,255,255);
	esat::DrawSolidPath(&viewer->triangle[0].x, 3);
	esat::DrawSolidPath(&console->triangle[0].x, 3);
}
void DrawTables(TableName* tablename){
	static int TriangleAnimation = 0;
	//Table Names
	struct TableName *currenttablename = tablenamelist;
	struct Tables *currenttable = tablelist;
	float kCharWidth = 9;
  scroll_offset_x = (float) TableScroll * kCharWidth * 5.0f; 
	//Draws the table and it's name
	while(currenttable != NULL){
		float draw_points[8];
		for(int i = 0; i < 4; i++){
				draw_points[i * 2] = currenttablename->points[i].x - scroll_offset_x;
				draw_points[i * 2 + 1] = currenttablename->points[i].y;
		}
		esat::DrawSetStrokeColor(255,50,50,80);
		esat::DrawSetFillColor(255,255,255,5);
		esat::DrawSolidPath(draw_points, 4);
		esat::DrawSetFillColor(255,255,255,255);
		esat::DrawText(currenttable->stringpoints.x - scroll_offset_x, currenttable->stringpoints.y, currenttable->table_namestring); 
	
		
		currenttablename = currenttablename->next;
		currenttable = currenttable->next;
	}
  currenttable = tablelist;
  //Draws the columns and their name
	while(currenttable != NULL){
		if(strcmp(CurrentTable, currenttable->table_namestring) == 0){
			struct Columns *currentcolumn = currenttable->columns;
			while(currentcolumn != NULL){
				esat::DrawSetStrokeColor(255,255,255,0);
				esat::DrawSetFillColor(0,0,0,0);
				esat::DrawSolidPath(&currentcolumn->points[0].x, 4);
				esat::DrawSetFillColor(255,255,255,255);
				esat::DrawText(currentcolumn->stringpoints.x, currentcolumn->stringpoints.y, currentcolumn->column_namestring);
				currentcolumn = currentcolumn->next;
			}
		}
		currenttable = currenttable->next;
	}

	currenttable = tablelist;
	//Draws the columns and their name
	while(currenttable != NULL){
		if(strcmp(CurrentTable, currenttable->table_namestring) == 0){
			struct Info *info = currenttable->info;
			float DataStartBaseY = 105.0f;
			float RowHeight = 20.0f;
			float ViewerBottomLimit = kWindowHeight / 2 - 15;
			while(info != NULL){
				int row = info->row_index;             
				int visible_row_index = row - InfoScroll;              
				if(visible_row_index >= 0 && visible_row_index < NumbOfLines){              
						float y = DataStartBaseY + (visible_row_index * RowHeight);
						esat::DrawSetStrokeColor(255,255,255,0);
						esat::DrawSetFillColor(0,0,0,0);
						esat::DrawSetFillColor(255,255,255,255);
						if(y < ViewerBottomLimit){
								esat::DrawText(info->stringpoints.x, y, info->data_namestring);
						}
				}
				//To paint a triangle down to say player can yo further DOWN in the scroll
				if(TriangleAnimation >= 45){	//If variable gets to 30 restart it
					TriangleAnimation = 0;
				}
				//If animation is less than 15 display It, so if animation will display half the time only
				if(TriangleAnimation <= 23){
					if(visible_row_index > 0 && visible_row_index > NumbOfLines - 1){
						//ARROW DOWN
						DrawTriangle(1, 10, kWindowHeight - 377, 0);
					}
				}
				
				info = info->next;
			}
		}
		currenttable = currenttable->next;
	}
	TriangleAnimation++;
}
void InfoScroller(){
	if(esat::IsSpecialKeyDown(esat::kSpecialKey_Down)){
		InfoScroll += 1;
		struct Tables *curtable = tablelist;
		while(curtable && strcmp(CurrentTable, curtable->table_namestring) != 0){
			curtable = curtable->next;
		}
		if(curtable){
			int max_scroll = curtable->row_ammount - NumbOfLines;
			if(max_scroll < 0){ 
				max_scroll = 0;
			}
			if(InfoScroll > max_scroll){
				InfoScroll = max_scroll;
			}
		}
	}
	if(esat::IsSpecialKeyDown(esat::kSpecialKey_Up)){
		InfoScroll -= 1;
		if(InfoScroll < 0){
			InfoScroll = 0;
		}
	}
	//Table names
	if(esat::IsSpecialKeyDown(esat::kSpecialKey_Right)){
			TableScroll += 1;
	}
	if(esat::IsSpecialKeyDown(esat::kSpecialKey_Left)){
			TableScroll -= 1;
	}
}

void ResetTables(Tables* currenttable,Columns* currentcolumn,TableName* currenttablename,Info* info){
	FreeTableList(currenttable);
	FreeColumnsList(currentcolumn);
	FreeInfoList(info);
	FreeTableNameList(currenttablename);
}

//Function to draw Query Historial, and every line of It, appart from animation the line that displays pos of writing |
void DrawQueryTextLine(Console* console, bool* iswriting){
	float lineX = (console)->terminal_text.x, lineY = (console)->terminal_text.y;

	float linePos = strlen(console->terminal_string) * 9;
	float lineOffset = linePos + (float) lineX + 2.5f;
	static int LineAnimation = 0;
	//If user if writing
	if(*iswriting){
		LineAnimation++;
		if(LineAnimation >= 30){	//If variable gets to 30 restart it
			LineAnimation = 0;
		}
		//If animation is less than 15 display It, so if animation will display half the time only
		if(LineAnimation <= 15){
			esat::DrawSetStrokeColor(255,255,255,255);
			esat::DrawLine(lineOffset, lineY - 12, lineOffset, lineY + 3);
		}
	}
}
//Function to draw Query result lines
void DrawResultText(Console* console){
  esat::DrawSetFillColor(255, 255, 255, 255);
  esat::DrawSetTextSize(console->fontsize);

  // Display 5 visible lines from the current offset
  for(int i = 0; i < LinesInScroll; i++){
    int currentLine = display_offset + i;
    if(currentLine >= total_results) break;
    esat::DrawText(console->result_text.x,
                   console->result_text.y + i * 20,
                   query_results[currentLine]);
  }
	//To paint a triangle down to say player can yo further DOWN in the scroll
	if(total_results > LinesInScroll && display_offset + LinesInScroll != total_results){
		//ARROW DOWN
		DrawTriangle(1, 15, kWindowHeight - 233, LinesInScroll * 20);
	}
	//To paint a triangle up to say player can yo further UP in the scroll
	if(total_results > LinesInScroll && display_offset > 0){
		//ARROW UP
		DrawTriangle(0, 15, kWindowHeight - 239, 0);
	}
}
//Function to process new lines in the query result lines
void HandleMouseScroll(){
	if(esat::MouseWheelY() < MouseY){
		MouseY = esat::MouseWheelY();
		if(display_offset + 1 < total_results - LinesInScroll + 1){
      display_offset++;
    }
	}
	if(esat::MouseWheelY() > MouseY){
		MouseY = esat::MouseWheelY();
		if(display_offset > 0){
			display_offset--;
      
    }
	}
}

//Function that returns submission from bbdd
static int callback(void *data, int argc, char **argv, char **azColName){

  if(total_results >= 256) return 0;

  char line[256] = {0};
  // Concatenate column names and their values in one line
  for(int i = 0; i < argc; i++){
    const char* col = azColName[i] ? azColName[i] : "(null)";
    const char* val = argv[i] ? argv[i] : "NULL";
		//To do not get out of line
    if(strlen(line) + strlen(col) + strlen(val) + 4 >= sizeof(line)) break;
    strcat(line, col);
    strcat(line, " = ");
    strcat(line, val);
    strcat(line, "  ");
  }

  snprintf(query_results[total_results], sizeof(query_results[total_results]), "%s", line);
  total_results++;
  return 0;
}
//Function that send Query writted to the bbdd
void SendQuery(Console* console, char* sql){
  sqlite3 *db;
  char *zErrMsg = 0;
  int rc;

  // Reset result buffer
  total_results = 0;
  display_offset = 0;
  for(int i = 0; i < 256; i++) query_results[i][0] = '\0';

  // Open database
  rc = sqlite3_open("assets/database/database.db", &db);
  if(rc){
    snprintf(query_results[0], sizeof(query_results[0]), "Can't open database: %s", sqlite3_errmsg(db));
    total_results = 1;
  } else {
    // Execute SQL statement
    rc = sqlite3_exec(db, sql, callback, console, &zErrMsg);
    if(rc != SQLITE_OK){
      snprintf(query_results[0], sizeof(query_results[0]), "SQL error: %s", zErrMsg);
      sqlite3_free(zErrMsg);
      total_results = 1;
    } else if(total_results == 0){
      snprintf(query_results[0], sizeof(query_results[0]), "Query executed successfully, no rows.");
      total_results = 1;
    }
    sqlite3_close(db);
  }
}

//Function that controls terminal query texting
int QueryTexting(Console* console, bool* iswriting,Tables* currenttable,Columns* currentcolumn,TableName* currenttablename,Info* info){
	static int stringpos = 0;
	//These two ifs controll whether its writing or not (If user has clicked in text window or not)
	if(MouseInSquare(console->terminal[0],console->terminal[2]) && esat::MouseButtonDown(0) && *iswriting == false){
		*iswriting = true;
	}
	if(!MouseInSquare(console->terminal[0],console->terminal[2]) && esat::MouseButtonDown(0) && *iswriting == true){
		*iswriting = false;
	}
	//These one controls Send Button -> If player has his mouse pointing to it and query line has something to send (Stringpos > 0)
	if(MouseInSquare(console->terminal[4],console->terminal[6])){
		if(esat::MouseButtonDown(0) && stringpos > 0){
			//This part is to set last char to \0
			console->terminal_string[stringpos + 1] = '\0';
			SendQuery(console,console->terminal_string);

			//Sound when button is pressed
			#ifdef WIN32
				soloud.play(audio[1], 1.0f /* volumen */, 0.0f /* pan */);
			#endif
			currenttable = tablelist;
			ResetTables(currenttable,currentcolumn,currenttablename,info);
			tablelist = NULL;
			tablenamelist = NULL;
      TableCount = 0;
			Sql();
			InitPoints(&currenttablename);
		}
	}
	//This part controlls every aspect of Typing
	if(*iswriting){
		char auxkey = '\0';
		auxkey = esat::GetNextPressedKey();
		// Fix layout mismatch for Spanish keyboard (ISO layout)
		switch(auxkey) {
			case '/': auxkey = '-'; break;   // "/" key becomes "-"
			case '-': auxkey = '\''; break;  // "-" key becomes "'"
			case '\'': auxkey = '/'; break;  // "'" key becomes "/"
			case ']': auxkey = '+'; break;   // "]" key becomes "+"
			case '[': auxkey = '`'; break;   // "[" key becomes "´"
		}
		if(auxkey != '\0' && stringpos < 200){
			if(esat::IsSpecialKeyPressed(esat::kSpecialKey_Shift)){
				switch(auxkey){
					case '1':{
						auxkey = '!';
						break;
					}
					case '2':{
						auxkey = '"';
						break;
					}
					case '4':{
						auxkey = '$';
						break;
					}
					case '5':{
						auxkey = '%';
						break;
					}
					case '6':{
						auxkey = '&';
						break;
					}
					case '7':{
						auxkey = '/';
						break;
					}
					case '8':{
						auxkey = '(';
						break;
					}
					case '9':{
						auxkey = ')';
						break;
					}
					case '0':{
						auxkey = '=';
						break;
					}
					case ',':{
						auxkey = ';';
						break;
					}
					case '.':{
						auxkey = ':';
						break;
					}
					case '-':{
						auxkey = '_';
						break;
					}
					case '\'':{
						auxkey = '?';
						break;
					}
					case '<':{
						auxkey = '>';
						break;
					}
					case '+':{
						auxkey = '*';
						break;
					}
				}
				console->terminal_string[stringpos] = auxkey;
				stringpos++;
			}else if(esat::IsSpecialKeyPressed(esat::kSpecialKey_Alt)){
				switch(auxkey){
					case '1':{
						auxkey = '|';
						break;
					}
					case '2':{
						auxkey = '@';
						break;
					}
					case '3':{
						auxkey = '#';
						break;
					}
					case '4':{
						auxkey = '~';
						break;
					}
					case '6':{
						//auxkey = '¬'; //This key doesn't work
						break;
					}
					case '+':{
						auxkey = ']';
						break;
					}
					case '`':{
						auxkey = '[';
						break;
					}
				}
				console->terminal_string[stringpos] = auxkey;
				stringpos++;
			}else{
				if(auxkey != ' ' && auxkey > 64 && auxkey < 91){
					auxkey += 32;
				}
				console->terminal_string[stringpos] = auxkey;
				stringpos++;
			}
		}
		if(esat::IsSpecialKeyDown(esat::kSpecialKey_Backspace) && stringpos > 0){
			stringpos--; 
			console->terminal_string[stringpos] = '\0';
    }
		esat::ResetBufferdKeyInput();
	}

	//If User if not writing also we reset buffer
	if(!*iswriting){
		esat::ResetBufferdKeyInput();
	}
	return stringpos;
}
void DrawTriangle(bool direction, float posX, float posY, int offsetY){
	if(direction){
		//To paint a triangle down to say player can yo further DOWN in the scroll
		//ARROW UP
		float points[6] = { 7 + posX,  8 + posY + offsetY,
												15 + posX,  8 + posY + offsetY, 
												11 + posX, 14 + posY + offsetY};
		esat::DrawSetStrokeColor(255,255,255,255);
		esat::DrawSolidPath(points, 3);
	}else{
		//To paint a triangle up to say player can yo further UP in the scroll
		//ARROW DOWN
		float points[6] = { 7 + posX,  8 + posY + offsetY + 5,
												15 + posX,  8 + posY + offsetY + 5, 
												11 + posX,  1 + posY + offsetY + 5};
		esat::DrawSetStrokeColor(255,255,255,255);
		esat::DrawSolidPath(points, 3);
	}
}
//If mouse is inside of the coords provided it returns a true
bool MouseInSquare(Vec2 point0, Vec2 point2){
	float mx = esat::MousePositionX();
  float my = esat::MousePositionY();
	if(	my > point0.y &&
			my < point2.y &&
			mx > point0.x &&
			mx < point2.x		){
		return true;
	}else{
		return false;
	}
} 
//Frees the info list
void FreeInfoList(struct Info* info){
    struct Info* tmp;
    while (info){
        tmp = info;
        info = info->next;
        free(tmp);
    }
}
//Frees the column list
void FreeColumnsList(struct Columns* col){
    struct Columns* tmp;
    while (col){
        tmp = col;
        col = col->next;
        free(tmp);
    }
}
//Frees the table list
void FreeTableList(struct Tables* tbl){
    struct Tables* tmp;
    while (tbl){
        tmp = tbl;
        tbl = tbl->next;
        FreeColumnsList(tmp->columns);
        FreeInfoList(tmp->info);
        free(tmp);
    }
}

//Frees the table name list
void FreeTableNameList(struct TableName* currenttable){
    struct TableName* tmp;
    while (currenttable != NULL){
        tmp = currenttable;
        currenttable = currenttable->next;
        free(tmp);
    }
}


void ClosePointers(Console* console, Viewer* viewer){
	free(console);
	free(viewer);
}

//Function that resets a tring to pure \0
void ResetString(char* string, int caracters){
  for(int i = 0; i < caracters - 1; i++){
    *(string + i) = '\0';
  }
}

void InitSound(){
#ifdef WIN32
    //Load audio
    soloud.init();
    audio[0].load("assets/audio/clickbutton1.mp3");
    audio[1].load("assets/audio/clickbutton2.mp3");
#endif
}


void FreeSound(){
#ifdef WIN32
   soloud.deinit();
#endif
}