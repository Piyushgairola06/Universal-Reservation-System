//api calls to communicate between frontend and backend
//By Piyush Gairola

#ifndef BACKEND_H //guards 
#define BACKEND_H //guards(so that it cannot be defined more than one time)

void backend_init();//loads data
//Basic functions for the backend of the project
int backend_book(const char *name, int age, const char *contact,int route_from, int route_to);


void backend_cancel(int reservation_id);

void backend_modify(int reservation_id,const char *newname, int newage, const char *newcontact);


int backend_search(int reservation_id);

void backend_assign_route(int reservation_id, int from, int to);

void backend_undo();


void backend_change_slots(int n);

void backend_get_confirmed_text(char *buf, int bufsize);
void backend_get_waitlist_text(char *buf, int bufsize);
void backend_get_slotmap_text(char *buf, int bufsize);
void backend_get_availability_text(char *buf, int bufsize);


void backend_save_all();//saves essential info to files before exiting the program

int backend_get_shortest_path_text(int from, int to,char *buf, int bufsize);

#endif
