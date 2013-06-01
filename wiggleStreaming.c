#include <stdlib.h>
#include <string.h>
#include "wiggleMultiplexer.h"

#define MAXLINE 10000

void printMultiplexer(FILE * dest, Multiplexer * multi) {
	int i; 
	fprintf(dest, "%s %i %i", multi->chrom, multi->start, multi->finish);
	fprintf(dest, "\t%f", multi->values[0]);
	for (i = 1; i < multi->count; i++) {	
		if (multi->inplay[i])
			fprintf(dest, " %lf", multi->values[i]);
		else
			fprintf(dest, " ");
	}
	fprintf(dest, "\n");
}

void streamMultiplexer(FILE * dest, Multiplexer * multi) {
	for (; !multi->done; popMultiplexer(multi))
		printMultiplexer(dest, multi);
}

void streamWiggleIteratorAtIndex(FILE * dest, WiggleIterator * iter, int index, int count) {
	streamMultiplexer(dest, newIteratorMultiplexer(iter, index, count));
}

// xstrtok version that handles null fields
char *xstrtok(char * line, char * delims)
{
	static char *saveline = NULL;
	char *p;
	int n;

	if (line != NULL)
		   saveline = line;

	// See if we have reached the end of the line 
	if (saveline == NULL || *saveline == '\0') 
		   return NULL;

	// return the number of characters that aren't delims 
	n = strcspn(saveline, delims);
	p = saveline; /*save start of this token*/

	saveline += n; /*bump past the delim*/

	if(*saveline != '\0') /*trash the delim if necessary*/
		   *saveline++ = '\0';

	return(p);
}

void popStreamingMultiplexer(Multiplexer * multi) {
	char line[MAXLINE];
	char *ptr;
	int counter = 0;

	if (multi->done)
		return;

	if (!fgets(line, MAXLINE, multi->file)) {
		multi->done = true;
		return;
	}

	line[strlen(line) - 1] = '\0';

	ptr = xstrtok(line, "\t");
	sscanf(ptr, "%s %i %i", multi->chrom, &(multi->start), &(multi->finish));
	while ((ptr = xstrtok(NULL, " "))) {
		if (ptr[0] == '\0') {
			multi->inplay[counter] = false;
		} else {
			multi->inplay[counter] = true;
			sscanf(ptr, "%lf", &(multi->values[counter]));
		}
		counter++;
	}

	if (counter != multi->count) {
		printf("Inconsistent number of columns in stream! Counted %i instead of %i\n", counter, multi->count);
		exit(1);
	}
}

Multiplexer * newStreamingMultiplexer(FILE * input) {
	char line[MAXLINE];
	char *ptr, *c;
	int counter = 0;

	Multiplexer * new = (Multiplexer *) calloc(1, sizeof(Multiplexer));
	new->pop = &popStreamingMultiplexer; 
	new->file = input;
	new->chrom = (char *) calloc(1000, sizeof(char));

	if (!fgets(line, MAXLINE, new->file)) {
		new->done = true;
		return new;
	}
	
	ptr = xstrtok(line, "\t\n");
	sscanf(ptr, "%s %i %i", new->chrom, &(new->start), &(new->finish));

	if((ptr = xstrtok(NULL, ""))) { 
		for (c = ptr; *c != '\0'; c++)
			if (*c == ' ')
				counter++; 

		new->count = counter + 1;
		new->inplay = (bool *) calloc(new->count, sizeof(bool));
		new->values = (double *) calloc(new->count, sizeof(double));

		ptr = xstrtok(ptr, " \n");
		if (ptr[0] == '\0') {
			new->inplay[0] = false;
		} else {
			new->inplay[0] = true;
			sscanf(ptr, "%lf", &(new->values[0]));
		}

		counter = 1;
		while ((ptr = xstrtok(NULL, " \n"))) {
			if (ptr[0] == '\0') {
				new->inplay[counter] = false;
			} else {
				new->inplay[counter] = true;
				sscanf(ptr, "%lf", &(new->values[counter]));
			}
			counter++;
		}
	} else {
		fprintf(stderr, "No columns in stream!");
		exit(1);
	}	
	return new;
}