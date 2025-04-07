#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_BOATS 120
#define MAX_NAME_LEN 128
#define MAX_LICENSE_LEN 16
#define MAX_LINE 256

#define RATE_SLIP     12.50
#define RATE_LAND     14.00
#define RATE_TRAILOR  25.00
#define RATE_STORAGE  11.20


// --- data definitions ---
// Boat: represents a single boat with name, size, location, and amount owed
// LocationDetail: stores specific info depending on boat's storage type
// BoatDatabase: holds the fleet (array of boats) and the current count
// BoatPtr: pointer to a Boat structure for flexibility in array management

typedef enum { SLIP, LAND, TRAILOR, STORAGE } LocationType;

typedef union {
    int slipNumber;
    char bayLetter;
    char trailorTag[MAX_LICENSE_LEN];
    int storageSpace;
} LocationDetail;

typedef struct {
    char name[MAX_NAME_LEN];
    float length;
    LocationType type;
    LocationDetail location;
    float amountOwed;
} Boat;

typedef Boat* BoatPtr;

typedef struct {
    BoatPtr boatList[MAX_BOATS];
    int boatCount;
} BoatDatabase;

//converts a string to lowercase for case-insensitive comparisons
void toLowerStr(char* dest, const char* src) {
    int i = 0;
    while (src[i] != '\0') {
        dest[i] = tolower(src[i]);
        i++;
    }
    dest[i] = '\0';
}

//compares two boat names case-insensitively
int namesEqual(const char* a, const char* b) {
    char lowerA[MAX_NAME_LEN], lowerB[MAX_NAME_LEN];
    toLowerStr(lowerA, a);
    toLowerStr(lowerB, b);
    return strcmp(lowerA, lowerB) == 0;
}

//compares boat names to sort them alphabetically
int compareBoats(const void* a, const void* b) {
    Boat* boatA = *(Boat**)a;
    Boat* boatB = *(Boat**)b;
    return strcasecmp(boatA->name, boatB->name);
}

//adds monthly charges based on the boat type and length
void applyMonthlyCharges(Boat* boat) {
    float rate;
    switch (boat->type) {
        case SLIP:     rate = RATE_SLIP; break;
        case LAND:     rate = RATE_LAND; break;
        case TRAILOR:  rate = RATE_TRAILOR; break;
        case STORAGE:  rate = RATE_STORAGE; break;
    }
    boat->amountOwed = boat->amountOwed + (rate * boat->length);
}

//loads boat data to a csv file
void loadCSV(BoatDatabase* db, const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Could not open %s for reading.\n", filename);
        return;
    }

    char line[MAX_LINE];
    while (fgets(line, sizeof(line), file) && db->boatCount < MAX_BOATS) {
        BoatPtr boat = malloc(sizeof(Boat));
        if (!boat) {
            fprintf(stderr, "Memory allocation failed.\n");
            continue;
        }

        char typeStr[16], detail[32];
            if (sscanf(line, "%127[^,],%f,%15[^,],%31[^,],%f",
                   boat->name, &boat->length, typeStr, detail, &boat->amountOwed) == 5) {

            if (strcasecmp(typeStr, "slip") == 0) {
                boat->type = SLIP;
                boat->location.slipNumber = atoi(detail);
            } else if (strcasecmp(typeStr, "land") == 0) {
                boat->type = LAND;
                boat->location.bayLetter = detail[0];
            } else if (strcasecmp(typeStr, "trailor") == 0) {
                boat->type = TRAILOR;
                strncpy(boat->location.trailorTag, detail, MAX_LICENSE_LEN);
            } else if (strcasecmp(typeStr, "storage") == 0) {
                boat->type = STORAGE;
                boat->location.storageSpace = atoi(detail);
            }

            db->boatList[db->boatCount] = boat;
            db->boatCount++;
        } else {
            free(boat);
        }
    }

    fclose(file);
    qsort(db->boatList, db->boatCount, sizeof(BoatPtr), compareBoats);
}

//saves all the boat data to a csv file
void saveCSV(BoatDatabase* db, const char* filename) {
    FILE* file = fopen(filename, "w");
    if (!file) {
        fprintf(stderr, "Could not open %s for writing.\n", filename);
        return;
    }

    for (int i = 0; i < db->boatCount; i++) {
        Boat* b = db->boatList[i];
        fprintf(file, "%s,%.0f,", b->name, b->length);
        switch (b->type) {
            case SLIP:    fprintf(file, "slip,%d", b->location.slipNumber); break;
            case LAND:    fprintf(file, "land,%c", b->location.bayLetter); break;
            case TRAILOR: fprintf(file, "trailor,%s", b->location.trailorTag); break;
            case STORAGE: fprintf(file, "storage,%d", b->location.storageSpace); break;
        }
        fprintf(file, ",%.2f\n", b->amountOwed);
    }
    fclose(file);
}

//prints the current sorted inventory of boats
void printInventory(BoatDatabase* db) {
    for (int i = 0; i < db->boatCount; i++) {
        Boat* b = db->boatList[i];
        printf("%-20s %4.0f' ", b->name, b->length);
        switch (b->type) {
            case SLIP:    printf("   slip   # %2d", b->location.slipNumber); break;
            case LAND:    printf("   land     %c", b->location.bayLetter); break;
            case TRAILOR: printf(" trailor %s", b->location.trailorTag); break;
            case STORAGE: printf(" storage  # %2d", b->location.storageSpace); break;
        }
        printf("   Owes $%7.2f\n", b->amountOwed);
    }
}

//adds a new boat from a csv-format string to the array of boats
void addBoat(BoatDatabase* db, const char* line) {
    if (db->boatCount >= MAX_BOATS) return;

    BoatPtr boat = malloc(sizeof(Boat));
    if (!boat) return;

    char typeStr[16], detail[32];
    if (sscanf(line, "%127[^,],%f,%15[^,],%31[^,],%f",
               boat->name, &boat->length, typeStr, detail, &boat->amountOwed) == 5) {

        if (strcasecmp(typeStr, "slip") == 0) {
            boat->type = SLIP;
            boat->location.slipNumber = atoi(detail);
        } else if (strcasecmp(typeStr, "land") == 0) {
            boat->type = LAND;
            boat->location.bayLetter = detail[0];
        } else if (strcasecmp(typeStr, "trailor") == 0) {
            boat->type = TRAILOR;
            strncpy(boat->location.trailorTag, detail, MAX_LICENSE_LEN);
        } else if (strcasecmp(typeStr, "storage") == 0) {
            boat->type = STORAGE;
            boat->location.storageSpace = atoi(detail);
        }

        db->boatList[db->boatCount] = boat;
        db->boatCount++;
        qsort(db->boatList, db->boatCount, sizeof(BoatPtr), compareBoats);
    } else {
        free(boat);
    }
}

//finds a boat by name (case-insensitive)
Boat* findBoat(BoatDatabase* db, const char* name) {
    for (int i = 0; i < db->boatCount; i++) {
        if (namesEqual(db->boatList[i]->name, name)) 
            return db->boatList[i];
    }
    return NULL;
}

//removes a boat by name and shifts remaining entries
void removeBoat(BoatDatabase* db, const char* name) {
    for (int i = 0; i < db->boatCount; i++) {
        if (namesEqual(db->boatList[i]->name, name)) {
            free(db->boatList[i]);
            for (int j = i; j < db->boatCount - 1; j++) {
                db->boatList[j] = db->boatList[j + 1];
            }
            db->boatCount--;
            return;
        }
    }
    printf("No boat with that name\n");
}

//processes a payment to a specific boat
void acceptPayment(BoatDatabase* db, const char* name, float amount) {
    Boat* boat = findBoat(db, name);
    if (!boat) {
        printf("No boat with that name\n");
        return;
    }
    if (amount > boat->amountOwed) {
        printf("That is more than the amount owed, $%.2f\n", boat->amountOwed);
        return;
    }
    boat->amountOwed = boat->amountOwed - amount;
}

//applies monthly charges to every boat in the array
void applyMonthlyToAll(BoatDatabase* db) {
    for (int i = 0; i < db->boatCount; i++) {
        applyMonthlyCharges(db->boatList[i]);
    }
}

//handles user interaction, menu logic, and overall program control
int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Error: Missing input file\n"
                        "Expected format: %s <BoatData.csv>\n", argv[0]);
        return 1;
    }
    
    BoatDatabase db = { .boatCount = 0 };

    loadCSV(&db, argv[1]);

    printf("Hi! Welcome to Emily's Boat Management System\n");
    printf("---------------------------------------------\n");
    
    char choice[6], input[MAX_LINE];

    while (1) {
        printf("\n(I)nventory, (A)dd, (R)emove, (P)ayment, (M)onth, e(X)it : ");
        fgets(choice, sizeof(choice), stdin);
        char c = tolower(choice[0]);

        //inventory
        if (c == 'i') {
            printInventory(&db);
        }

        //add boat
        else if (c == 'a') {
            printf("Please enter the boat data in CSV format                 : ");
            fgets(input, sizeof(input), stdin);
            input[strcspn(input, "\n")] = 0;
            addBoat(&db, input);
        }

        //remove boat
        else if (c == 'r') {
            printf("Please enter the boat name                               : ");
            fgets(input, sizeof(input), stdin);
            input[strcspn(input, "\n")] = 0;
            removeBoat(&db, input);
        }

        //make payment
        else if (c == 'p') {
            printf("Please enter the boat name                               : ");
            fgets(input, sizeof(input), stdin);
            input[strcspn(input, "\n")] = 0;
            Boat* b = findBoat(&db, input);
            if (!b) {
                printf("No boat with that name\n");
                continue;
            }
            printf("Please enter the amount to be paid                       : ");
            fgets(choice, sizeof(choice), stdin);
            float amt = atof(choice);
            acceptPayment(&db, input, amt);
        }

        //charge monthly amount to all boats
        else if (c == 'm') {
            applyMonthlyToAll(&db);
        }

        //exit program
        else if (c == 'x') {
            break;
        }

        //invalid choice
        else {
            printf("Invalid option %c\n", choice[0]);
        }
    }

    saveCSV(&db, argv[1]);
    for (int i = 0; i < db.boatCount; i++) {
        free(db.boatList[i]);
    }
    printf("\nExiting Emily's Boat Management System! Have a great day!\n");
    return 0;
}

