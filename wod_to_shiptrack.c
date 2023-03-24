#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define NUM_CASTS_TO_READ 100
#define KEYS {"Latitude", "Longitude", "Year", "Month", "Day"}
#define NUM_KEYS 5
#define NUM_CSVS 15
#define INITIAL_CAST_BUF_SIZE 4096

char **getNthCastValues(size_t n, FILE* fp, char **keys, size_t numkeys) {
	char **values = malloc(numkeys * sizeof(char *));
	char *linetemp;
	size_t linetemplen = 0;

	size_t castcounter = 0;
	do {
		linetemp = NULL;
		if (getline(&linetemp, &linetemplen, fp) == -1) {
			return NULL;
		}
		if (!strncmp(linetemp, "CAST", 4)) {
			castcounter++;
		}
		free(linetemp);
	} while (castcounter <= n);
	linetemp = NULL;
	linetemplen = 0;
	
	do {
		if (linetemp) {
			free(linetemp);
		}
		linetemp = NULL;
		getline(&linetemp, &linetemplen, fp);
		for (size_t i = 0; i < numkeys; i++) {
			if (!strncmp(linetemp, keys[i], strlen(keys[i]))) {
				// find next non-comma, non-whitespace value and cpy it
				values[i] = linetemp + strlen(keys[i]);
				values[i] += strspn(values[i], ", \t");
				values[i][strcspn(values[i], ", \t")] = '\0';
				values[i] = strdup(values[i]);
				break;
			}
		}
	} while (strncmp(linetemp, "CAST", 4));
	return values;
}

int main(int argc, char** argv) {
/*	FILE *csv = fopen(argv[1], "r");
	rewind(csv);
	char *keys[NUM_KEYS] = KEYS;
	char **values = getNthCastValues(0, csv, &keys[0], NUM_KEYS);
	for (size_t i = 0; i < 2; i++) {
		printf("%s, ", values[i]);
		free(values[i]);
	}
	printf("\n");
	fclose(csv);
*/
	FILE *filelist = fopen(argv[1], "r");
	char *filepaths[NUM_CSVS];
	size_t dummysize;
	for (size_t i = 0; i < NUM_CSVS; i++) {
		filepaths[i] = NULL;
		getline(&filepaths[i], &dummysize, filelist);
		filepaths[i][strlen(filepaths[i]) - 1] = '\0';
	}
	FILE *csv;
	FILE *dst = fopen(argv[2], "w");
	char *keys[NUM_KEYS] = KEYS;
	char **values;

	const char* seabassheader = "/begin_header\n/missing=-999\n/delimiter=space\n/fields=lat,lon,date\n/units=deg,deg\n/end_header\n";
	fputs(seabassheader, dst);
	char datestring[9];
	datestring[8] = '\0';
	for (size_t csvidx = 0; csvidx < NUM_CSVS; csvidx++) {
		csv = fopen(filepaths[csvidx], "r");
		for (size_t cast = 0; (values = getNthCastValues(cast, csv, keys, NUM_KEYS)); cast++) {
			if (atoi(values[2]) != atoi(argv[3]) || atoi(values[3]) != atoi(argv[4])) {
				continue;
			}
			for (size_t keyidx = 0; keyidx < NUM_KEYS; keyidx++) {
				if (!strcmp(keys[keyidx], "Year")) {
					strncpy(&datestring[0], values[keyidx], 4);
				}
				else if (!strcmp(keys[keyidx], "Month")) {
					if (strlen(values[keyidx]) == 1) {
						datestring[4] = '0';
						datestring[5] = values[keyidx][0];
					}
					else {
						strncpy(&datestring[4], values[keyidx], 2);
					}
				}
				else if (!strcmp(keys[keyidx], "Day")) {
					if (strlen(values[keyidx]) == 1) {
						datestring[6] = '0';
						datestring[7] = values[keyidx][0];
					}
					else {
						strncpy(&datestring[6], values[keyidx], 2);
					}
				}
				else {
					fputs(values[keyidx], dst);
					fputs("\t", dst);
				}
			}
			fputs(&datestring[0], dst);
			fputs("\n", dst);

			free(values);
		}
		// free(filepaths[csvidx]);		
		fclose(csv);
	}

	fclose(dst);
	return 0;
}
