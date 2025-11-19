/* frontend.c
   Full GUI for Universal Reservation System (with shortest path)
   Uses Raylib for UI.
   made by Piyush Gairola
*/

#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "backend.h"

#define SCREEN_W 900
#define SCREEN_H 650

typedef struct {
    Rectangle rec;
    const char *label;
} Button;

void DrawButton(Button b) {
    DrawRectangleRec(b.rec, LIGHTGRAY);
    DrawRectangleLines(b.rec.x, b.rec.y, b.rec.width, b.rec.height, BLACK);
    int textWidth = MeasureText(b.label, 20);
    DrawText(b.label, b.rec.x + (b.rec.width - textWidth)/2, b.rec.y + (b.rec.height - 20)/2, 20, BLACK);
}

/* Popup modal input window with multiple fields */
static int modal_input(const char *title, const char *fields[], int field_count, char out[][128]) {
    int done = 0, canceled = 0, active = 0;
    char inputs[8][128] = {0};

    while (!done && !canceled && !WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);

        DrawRectangle(100, 80, SCREEN_W-200, SCREEN_H-160, (Color){240,240,250,255});
        DrawText(title, 120, 90, 22, BLACK);

        int baseY = 130;
        for (int i = 0; i < field_count; i++) {
            DrawText(fields[i], 120, baseY + i*50, 18, DARKGRAY);

            Rectangle box = {300, baseY + i*50 -4, 420, 30};
            DrawRectangleRec(box, WHITE);
            DrawRectangleLines(box.x, box.y, box.width, box.height, GRAY);
            DrawText(inputs[i], box.x + 10, box.y + 6, 18, BLACK);

            /* highlight active field */
            if (i == active) {
                DrawRectangleLines(box.x-2, box.y-2, box.width+4, box.height+4, BLUE);
            }
        }

        Rectangle okR = {SCREEN_W-360, SCREEN_H-130, 120, 36};
        Rectangle cancelR = {SCREEN_W-220, SCREEN_H-130, 120, 36};

        DrawRectangleRec(okR, LIGHTGRAY);
        DrawRectangleRec(cancelR, LIGHTGRAY);
        DrawText("OK", okR.x+40, okR.y+6, 20, BLACK);
        DrawText("Cancel", cancelR.x+20, cancelR.y+6, 20, BLACK);

        EndDrawing();

        /* text input */
        int key = GetCharPressed();
        while (key > 0) {
            if (key >= 32 && key <= 125) {
                int len = strlen(inputs[active]);
                if (len < 120) {
                    inputs[active][len] = (char)key;
                    inputs[active][len+1] = '\0';
                }
            }
            key = GetCharPressed();
        }

        if (IsKeyPressed(KEY_TAB)) active = (active + 1) % field_count;

        if (IsKeyPressed(KEY_BACKSPACE)) {
            int len = strlen(inputs[active]);
            if (len > 0) inputs[active][len-1] = '\0';
        }
        if (IsKeyPressed(KEY_ENTER)) done = 1;

        /* mouse */
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Vector2 m = GetMousePosition();
            if (CheckCollisionPointRec(m, okR)) done = 1;
            if (CheckCollisionPointRec(m, cancelR)) canceled = 1;

            for (int i = 0; i < field_count; i++) {
                Rectangle rr = {300, 130 + i*50 - 4, 420, 30};
                if (CheckCollisionPointRec(m, rr)) active = i;
            }
        }
    }

    if (done) {
        for (int i = 0; i < field_count; i++) {
            strncpy(out[i], inputs[i], 127);
            out[i][127] = '\0';
        }
        return 1;
    }
    return 0;
}

int main(void) {
    InitWindow(SCREEN_W, SCREEN_H, "Universal Reservation - Full GUI");
    SetTargetFPS(60);

    backend_init();

    /* 13 buttons (with shortest path feature) */
    const char *labels[13] = {
        "Book Ticket","Cancel Ticket","Modify","Search",
        "Show Confirmed","Show Waitlist","Slot Map","Availability",
        "Undo Last","Change Slots","Assign Route","Shortest Path","Exit"
    };

    Button btns[13];
    int i = 0;

    /* Button layout: 4 rows x 4 cols space (we only use 13) */
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            if (i >= 13) break;
            btns[i].rec = (Rectangle){40 + c*210, 60 + r*120, 180, 80};
            btns[i].label = labels[i];
            i++;
        }
    }

    char output_buf[8192] = {0};

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground((Color){25,28,34,255});

        DrawText("UNIVERSAL RESERVATION SYSTEM", 40, 10, 28, RAYWHITE);
        DrawText("Enhanced GUI + Shortest Path", 40, 40, 16, LIGHTGRAY);

        for (int k = 0; k < 13; k++) DrawButton(btns[k]);

        /* Output panel */
        DrawRectangle(40, SCREEN_H-130, SCREEN_W-80, 110, (Color){14,14,18,255});
        DrawRectangleLines(40, SCREEN_H-130, SCREEN_W-80, 110, DARKGRAY);
        DrawText("Output:", 50, SCREEN_H-120, 18, LIGHTGRAY);

        DrawTextPro(GetFontDefault(), output_buf, 
                    (Vector2){50, SCREEN_H-95}, 
                    (Vector2){0,0}, 0, 16, 2, LIGHTGRAY);

        EndDrawing();

        /* Check button clicks */
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Vector2 m = GetMousePosition();

            for (int k = 0; k < 13; k++) {
                if (CheckCollisionPointRec(m, btns[k].rec)) {

                    /* -------------------------- BOOK ------------------------- */
                    if (strcmp(btns[k].label, "Book Ticket") == 0) {
                        const char *fields[] = {
                            "Name:", "Age:", "Contact:", 
                            "Route From (0=Delhi..5=Bangalore):", 
                            "Route To (0=Delhi..5=Bangalore):"
                        };
                        char outs[5][128];
                        if (modal_input("Book Ticket", fields, 5, outs)) {
                            int id = backend_book(
                                outs[0], atoi(outs[1]), outs[2], 
                                atoi(outs[3]), atoi(outs[4])
                            );
                            if (id == -1)
                                snprintf(output_buf, sizeof(output_buf), "Booking failed. Invalid route or no path.\n");
                            else
                                snprintf(output_buf, sizeof(output_buf), "Booked successfully. ID: %d\n", id);

                            backend_save_all();
                        }
                    }

                    /* -------------------------- CANCEL ------------------------- */
                    else if (strcmp(btns[k].label, "Cancel Ticket") == 0) {
                        const char *fields[] = {"Reservation ID:"};
                        char outs[1][128];
                        if (modal_input("Cancel Booking", fields, 1, outs)) {
                            backend_cancel(atoi(outs[0]));
                            snprintf(output_buf, sizeof(output_buf), "Cancellation done.\n");
                            backend_save_all();
                        }
                    }

                    /* -------------------------- MODIFY ------------------------- */
                    else if (strcmp(btns[k].label, "Modify") == 0) {
                        const char *fields[] = {"Reservation ID:", "New Name:", "New Age:", "New Contact:"};
                        char outs[4][128];
                        if (modal_input("Modify Details", fields, 4, outs)) {
                            backend_modify(
                                atoi(outs[0]),
                                outs[1],
                                atoi(outs[2]),
                                outs[3]
                            );
                            snprintf(output_buf, sizeof(output_buf), "Modification complete.\n");
                            backend_save_all();
                        }
                    }

                    /* -------------------------- SEARCH ------------------------- */
                    else if (strcmp(btns[k].label, "Search") == 0) {
                        const char *fields[] = {"Reservation ID:"};
                        char outs[1][128];
                        if (modal_input("Search Reservation", fields, 1, outs)) {
                            int res = backend_search(atoi(outs[0]));
                            if (res == 1) snprintf(output_buf, sizeof(output_buf), "Status: CONFIRMED\n");
                            else if (res == 2) snprintf(output_buf, sizeof(output_buf), "Status: WAITLIST\n");
                            else snprintf(output_buf, sizeof(output_buf), "NOT FOUND\n");
                        }
                    }

                    /* --------------------- SHOW CONFIRMED ---------------------- */
                    else if (strcmp(btns[k].label, "Show Confirmed") == 0) {
                        backend_get_confirmed_text(output_buf, sizeof(output_buf));
                    }

                    /* --------------------- SHOW WAITLIST ------------------------- */
                    else if (strcmp(btns[k].label, "Show Waitlist") == 0) {
                        backend_get_waitlist_text(output_buf, sizeof(output_buf));
                    }

                    /* --------------------- SLOT MAP ---------------------------- */
                    else if (strcmp(btns[k].label, "Slot Map") == 0) {
                        backend_get_slotmap_text(output_buf, sizeof(output_buf));
                    }

                    /* --------------------- AVAILABILITY ------------------------- */
                    else if (strcmp(btns[k].label, "Availability") == 0) {
                        backend_get_availability_text(output_buf, sizeof(output_buf));
                    }

                    /* ----------------------- UNDO LAST -------------------------- */
                    else if (strcmp(btns[k].label, "Undo Last") == 0) {
                        backend_undo();
                        snprintf(output_buf, sizeof(output_buf), "Undo complete.\n");
                        backend_save_all();
                    }

                    /* ----------------------- CHANGE SLOTS ----------------------- */
                    else if (strcmp(btns[k].label, "Change Slots") == 0) {
                        const char *fields[] = {"New Total Slots:"};
                        char outs[1][128];
                        if (modal_input("Change Slots", fields, 1, outs)) {
                            backend_change_slots(atoi(outs[0]));
                            snprintf(output_buf, sizeof(output_buf), "Slot count updated.\n");
                        }
                    }

                    /* ----------------------- ASSIGN ROUTE ------------------------ */
                    else if (strcmp(btns[k].label, "Assign Route") == 0) {
                        const char *fields[] = {
                            "Reservation ID:",
                            "From (0-5):", 
                            "To (0-5):"
                        };
                        char outs[3][128];
                        if (modal_input("Assign Route", fields, 3, outs)) {
                            backend_assign_route(
                                atoi(outs[0]),
                                atoi(outs[1]),
                                atoi(outs[2])
                            );
                            snprintf(output_buf, sizeof(output_buf), "Route assigned (if valid).\n");
                            backend_save_all();
                        }
                    }

                    /* ----------------------- SHORTEST PATH ------------------------ */
                    else if (strcmp(btns[k].label, "Shortest Path") == 0) {
                        const char *fields[] = {
                            "From (0=Delhi..5=Bangalore):",
                            "To (0=Delhi..5=Bangalore):"
                        };
                        char outs[2][128];
                        if (modal_input("Find Shortest Path", fields, 2, outs)) {
                            char spbuf[1024];
                            int res = backend_get_shortest_path_text(
                                atoi(outs[0]), atoi(outs[1]),
                                spbuf, sizeof(spbuf)
                            );
                            snprintf(output_buf, sizeof(output_buf), "%s", spbuf);
                        }
                    }

                    /* ----------------------------- EXIT ---------------------------- */
                    else if (strcmp(btns[k].label, "Exit") == 0) {
                        backend_save_all();
                        CloseWindow();
                        return 0;
                    }
                }
            }
        }
    }

    backend_save_all();
    CloseWindow();
    return 0;
}
