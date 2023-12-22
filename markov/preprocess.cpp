#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>

int main() {
	{
		const char* fn = "romeo-and-juliet.txt";
		
		FILE* fin = fopen(fn, "r");
		if (fin == nullptr) {
			printf("Failed to open file.");
			exit(1);
		}
		
		fseek(fin, 0, SEEK_END);
		int file_size = ftell(fin);
		rewind(fin);
		
		char* text = new char[file_size + 4000];
		int bytes_read = fread(text, 1, file_size, fin);
		fclose(fin);
		
		// Counts the number of lines and checks the bracket depth (text enclosed between '[' and ']' is removed).
		printf("Read %d of %d bytes from %s.\n", bytes_read, file_size, fn);
		int bracket_depth = 0;
		int max_bracket_depth = 0;
		int num_lines = 0;
		for (int i = 0; i < bytes_read; i++) {
			if (text[i] == '\n') num_lines++;
			if (text[i] == '[') bracket_depth++;
			if (text[i] == ']') bracket_depth--;
			
			if (bracket_depth < 0) {
				printf("Improper bracket closure on line %d.\n", num_lines+1);
				exit(1);
			}
			
			if (bracket_depth > max_bracket_depth) {
				max_bracket_depth = bracket_depth;
			}
		}
		printf("Max bracket depth is %d.\n", max_bracket_depth);
		if (max_bracket_depth > 1) {
			printf("Bracket depth is too high.\n"); // TODO: Remove after adding support for nested bracket removal.
			exit(1);
		}
		if (bracket_depth > 0) {
			printf("%d unclosed opening bracket(s).\n", bracket_depth);
			exit(1);
		}
		
		// Remove text enclosed in [].
		int start;
		for (int i = 0; i < bytes_read; i++) {
			if (text[i] == '[') start = i;
			if (text[i] == ']') {
				int end = i;
				int num_bytes_removed = i - start + 1;
				
				memmove(text + start, text + end + 1, bytes_read - i - 1);
				bytes_read -= num_bytes_removed;
				i = start;
			}
		}
		
		// Insert newlines after character names whose lines do not start on the next line beneath their name
		bool is_on_new_line = true;
		bool is_on_name = false;
		int num_chars = 0;
		int last_space = 0;
		for (int i = 0; i < bytes_read; i++) {
			if (is_on_new_line) {
				if (text[i] == '\n') {
					continue;
				}
				
				if (isupper(text[i])) {
					is_on_name = true;
					num_chars = 1;
				}
				is_on_new_line = false;
			}
			
			else if (is_on_name) {
				// TODO: Instead of replacing a character to produce the newline, insertion would be more reliable. Perhaps conditional insertion, iff the next character is not whitespace.
				if (isupper(text[i])) {
					num_chars++;
				}
				else if (text[i] == '\n') {
					is_on_name = false;
					is_on_new_line = true;
				}
				else if (text[i] == ' ' && i > 0 && text[i-1] != 'I' && text[i-1] != 'A') {
					last_space = i;
				}
				else {
					if (num_chars > 1 && last_space != 0) { // NOTE: If no space occurs prior to the detection of the first non-uppercase character bar space & newline, no newline will be written.
						text[last_space] = '\n';
						last_space = 0;
					}
					is_on_name = false;
				}
			}
			
			else {
				if (text[i] == '\n') is_on_new_line = true;
			}
		}
		
		for (int i = 0; i < bytes_read-2; i++) {
			if (text[i] == '\n' && isupper(text[i+1]) && isupper(text[i+2])) {
				int ins = i + 1;
				memmove(text + ins + 1, text + ins, bytes_read - ins);
				text[ins] = '>';
				bytes_read++;
			}
		}
		
		// Number of lines must be recounted.
		num_lines = 0;
		for (int i = 0; i < bytes_read; i++) {
			if (text[i] == '\n') num_lines++;
		}
		printf("Counted %d lines after character preprocessing.\n", num_lines);
		
		// Build index for start of all lines.
		// Newlines are replaced with null-terminating characters, so that the read text becomes an array of lines.
		char** lines = new char*[num_lines];
		lines[0] = text;
		int lines_found = 1;
		for (int i = 0; i < bytes_read-1; i++) {
			if (text[i] == '\n') {
				text[i] = '\0';
				lines[lines_found] = text + i + 1;
				lines_found++;
			}
		}
		text[bytes_read-1] = '\0';
		
		// Remove blank lines. Removes consecutive blank lines simultaneously.
		for (int l = 0; l < num_lines; l++) {
			if (lines[l][0] == '\0') {
				int m;
				for (m = l+1; m < num_lines && lines[m][0] == '\0'; m++);
				
				int start = l;
				int end = m - 1;
				int num_lines_removed = end - start + 1;
				int bytes_copied = (num_lines - end - 1) * sizeof(char*);
				
				if (bytes_copied > 0) {
					memmove(lines + start, lines + end + 1, bytes_copied);
				}
				num_lines -= num_lines_removed;
			}
		}
		
		// Save preprocessed.
		FILE* fout = fopen("preprocessed.txt", "w");
		if (fout == nullptr) {
			printf("Could not open file to write preprocessed data. Skipping.\n");
		}
		else {
			int num_bytes_written;
			for (int l = 0; l < num_lines; l++) {
				int len;
				for (len = 0; lines[l][len] != '\0'; len++);
				
				num_bytes_written += fwrite(lines[l], 1, len, fout);
				num_bytes_written += fwrite("\n", 1, 1, fout);
			}
			fclose(fout);
			
			printf("Wrote %d bytes to preprocessed.txt\n", num_bytes_written);
		}
		
		delete[] lines;
		delete[] text;
	}
	
	printf("Done.\n");
	return 0;
}