#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_STACK 100

struct customer {
    int reservation_id;
    char name[50];
    int age;
    char contact[15];
    int slot_number;
    struct customer *next;
};

struct customer *confirmed_list = NULL; 
struct customer *waitlist = NULL;

int total_slots = 5;
int booked_slots = 0;
int next_reservation_id = 1000;


struct customer undo_stack[MAX_STACK];
int top = -1;

void push_undo(struct customer c) {
    if (top >= MAX_STACK - 1)
        printf("\nStack full, cannot record further actions.\n");
    else
        undo_stack[++top] = c;
}

void undo_last_booking();


void insert_customer(int reservation_id, char name[], int age, char contact[], int slot_number) {
    struct customer *new = (struct customer *)malloc(sizeof(struct customer));
    new->reservation_id = reservation_id;
    strcpy(new->name, name);
    new->age = age;
    strcpy(new->contact, contact);
    new->slot_number = slot_number;
    new->next = NULL;

    if (confirmed_list == NULL)
        confirmed_list = new;
    else {
        struct customer *temp = confirmed_list;
        while (temp->next != NULL)
            temp = temp->next;
        temp->next = new;
    }
}


void delete_customer(int reservation_id) {
    struct customer *temp = confirmed_list, *prev = NULL;

    if (temp == NULL)
        return;

    if (temp->reservation_id == reservation_id) {
        confirmed_list = temp->next;
        free(temp);
        printf("\n Customer record %d deleted.\n", reservation_id);
        booked_slots--;
        return;
    }

    while (temp != NULL && temp->reservation_id != reservation_id) {
        prev = temp;
        temp = temp->next;
    }

    if (temp == NULL) return;

    prev->next = temp->next;
    free(temp);
    printf("\n Customer record %d deleted.\n", reservation_id);
    booked_slots--;
}


void show_confirmed_list() {
    printf("\n CONFIRMED RESERVATIONS \n");
    if (confirmed_list == NULL)
        printf("No confirmed reservations found.\n");
    else {
        struct customer *temp = confirmed_list;
        while (temp != NULL) {
            printf("ID:%d | Name:%s | Age:%d | Contact:%s | Slot:%d\n",
                   temp->reservation_id, temp->name, temp->age, temp->contact, temp->slot_number);
            temp = temp->next;
        }
    }
}


void enqueue_waitlist(int reservation_id, char name[], int age, char contact[]) {
    struct customer *new_record = (struct customer *)malloc(sizeof(struct customer));
    new_record->reservation_id = reservation_id;
    strcpy(new_record->name, name);
    new_record->age = age;
    strcpy(new_record->contact, contact);
    new_record->slot_number = -1;
    new_record->next = NULL;

    if (waitlist == NULL)
        waitlist = new_record;
    else {
        struct customer *temp = waitlist;
        while (temp->next != NULL)
            temp = temp->next;
        temp->next = new_record;
    }
}


void dequeue_waitlist() {
    if (waitlist == NULL) {
        printf("\nWaitlist empty.\n");
        return;
    }

    struct customer *temp = waitlist;
    waitlist = waitlist->next;

    printf("\n Waitlist customer %s (ID:%d) promoted to confirmed.\n",
           temp->name, temp->reservation_id);
    free(temp);
    }


void show_waitlist() {
    printf("\n WAITLIST \n");
    if (waitlist == NULL)
        printf("Waitlist is empty.\n");
    else {
        struct customer *temp = waitlist;
        while (temp != NULL) {
            printf("ID:%d | Name:%s | Age:%d | Contact:%s | Status:WAITLISTED\n",
                   temp->reservation_id, temp->name, temp->age, temp->contact);
            temp = temp->next;
        }
    }
}


void book_reservation(char name[], int age, char contact[]) {
    int reservation_id = next_reservation_id++;

    if (booked_slots < total_slots) {
        insert_customer(reservation_id, name, age, contact, ++booked_slots);
        printf("\nReservation confirmed for %s (Slot %d)\n", name, booked_slots);
        printf("Your Reservation ID is: %d\n", reservation_id);

        struct customer temp;
        temp.reservation_id = reservation_id;
        strcpy(temp.name, name);
        temp.age = age;
        strcpy(temp.contact, contact);
        temp.slot_number = booked_slots;
        push_undo(temp);
    } else {
        enqueue_waitlist(reservation_id, name, age, contact);
        printf("\nAll slots full. %s added to WAITLIST.\n", name);
        printf("Your Reservation ID is: %d\n", reservation_id);
    }
}


void cancel_reservation(int reservation_id) {
    delete_customer(reservation_id);
    if (waitlist != NULL) {
        struct customer *temp = waitlist;
        insert_customer(temp->reservation_id, temp->name, temp->age, temp->contact, ++booked_slots);
        dequeue_waitlist();
    }
}


void undo_last_booking() {
    if (top < 0) {
        printf("\nNo recent booking to undo.\n");
        return;
    }

    struct customer last = undo_stack[top--];
    printf("\n Undoing last booking: %s (ID:%d)\n", last.name, last.reservation_id);
    cancel_reservation(last.reservation_id);
}


void modify_customer(int reservation_id) {
    struct customer *temp = confirmed_list;
    while (temp != NULL && temp->reservation_id != reservation_id)
        temp = temp->next;
    if (temp == NULL) temp = waitlist;

    while (temp != NULL && temp->reservation_id != reservation_id)
        temp = temp->next;

    if (temp == NULL) {
        printf("\nNo record found with ID %d.\n", reservation_id);
        return;
    }

    printf("\nEditing details for %s:\n", temp->name);
    printf("Enter new name: "); scanf("%s", temp->name);
    printf("Enter new age: "); scanf("%d", &temp->age);
    printf("Enter new contact: "); scanf("%s", temp->contact);

    printf("\nRecord updated successfully!\n");
}


void search_customer(int reservation_id) {
    struct customer *temp = confirmed_list;
    while (temp != NULL) {
        if (temp->reservation_id == reservation_id) {
            printf("\nFOUND in CONFIRMED LIST:\n");
            printf("ID:%d | Name:%s | Age:%d | Contact:%s | Slot:%d\n",
                   temp->reservation_id, temp->name, temp->age, temp->contact, temp->slot_number);
            return;
        }
        temp = temp->next;
    }

    temp = waitlist;
    while (temp != NULL) {
        if (temp->reservation_id == reservation_id) {
            printf("\nFOUND in WAITLIST:\n");
            printf("ID:%d | Name:%s | Age:%d | Contact:%s | Status:WAITLISTED\n",
                   temp->reservation_id, temp->name, temp->age, temp->contact);
            return;
        }
        temp = temp->next;
    }

    printf("\nNo record found for Reservation ID: %d\n", reservation_id);
}


void show_slot_map() {
    printf("\n SLOT MAP \n");
    int i;
    for (i = 1; i <= total_slots; i++) {
        struct customer *temp = confirmed_list;
        int found = 0;
        while (temp != NULL) {
            if (temp->slot_number == i) {
                printf("Slot %d - %s\n", i, temp->name);
                found = 1;
                break;
            }
            temp = temp->next;
        }
        if (!found)
            printf("Slot %d - Available\n", i);
    }
}


void check_availability() {
    printf("\nTotal Slots: %d\n", total_slots);
    printf("Booked Slots: %d\n", booked_slots);
    printf("Available Slots: %d\n", total_slots - booked_slots);

    int wait_count = 0;
    struct customer *temp = waitlist;
    while (temp != NULL) {
        wait_count++;
        temp = temp->next;
    }
    printf("Waitlisted: %d\n", wait_count);
}


void change_total_slots() {
    int new_count;
    printf("Enter new total slot count: ");
    scanf("%d", &new_count);

    if (new_count < booked_slots) {
        printf("\nCannot reduce slots below current booked count (%d).\n", booked_slots);
        return;
    }

    total_slots = new_count;
    printf("\nTotal slots updated to %d.\n", total_slots);
}


void menu() {
    int choice, id, age;
    char name[50], contact[15];

    do {
        printf("\n\n    UNIVERSAL RESERVATION SYSTEM  \n");
        printf("1. New Reservation\n");
        printf("2. Cancel Reservation\n");
        printf("3. Modify Customer Details\n");
        printf("4. Search Reservation\n");
        printf("5. Show Confirmed List\n");
        printf("6. Show Waitlist\n");
        printf("7. Show Slot Map\n");
        printf("8. Check Availability\n");
        printf("9. Change Total Slots\n");
        printf("10. Undo Last Booking\n");
        printf("11. Exit\n");
        printf("Enter choice: ");
        scanf("%d", &choice);

        switch (choice) {
            case 1:
                printf("Enter Name: "); scanf("%s", name);
                printf("Enter Age: "); scanf("%d", &age);
                printf("Enter Contact: "); scanf("%s", contact);
                book_reservation(name, age, contact);
                break;

            case 2:
                printf("Enter Reservation ID to cancel: ");
                scanf("%d", &id);
                cancel_reservation(id);
                break;

            case 3:
                printf("Enter Reservation ID to modify: ");
                scanf("%d", &id);
                modify_customer(id);
                break;

            case 4:
                printf("Enter Reservation ID to search: ");
                scanf("%d", &id);
                search_customer(id);
                break;

            case 5:
                show_confirmed_list();
                break;

            case 6:
                show_waitlist();
                break;

            case 7:
                show_slot_map();
                break;

            case 8:
                check_availability();
                break;

            case 9:
                change_total_slots();
                break;

            case 10:
                undo_last_booking();
                break;

            case 11:
                printf("\nExiting system...\n");
                break;

            default:
                printf("\nInvalid choice. Please try again.\n");
        }
    } while (choice != 11);
}

int main() {
    menu();
    return 0;
}