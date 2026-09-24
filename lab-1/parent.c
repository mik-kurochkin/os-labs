#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <limits.h>

int main() {
	// Determining the path to the directory
	char path[PATH_MAX];
	ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
	if (len == -1) {
		const char msg[] = "error: failed to read full program path\n";
		write(STDERR_FILENO, msg, sizeof(msg) - 1);
		exit(EXIT_FAILURE);
	} else {
		while(path[len] != '/') {
			len--;
		}
		path[len] = '\0';
	}

	// Create parent_to_child pipe
	int parent_to_child[2];
	if (pipe(parent_to_child) == -1) {
		const char msg[] = "error: failed to create a 'parent_to_child' pipe\n";
		write(STDERR_FILENO, msg, sizeof(msg) - 1);
		exit(EXIT_FAILURE);
	}

	// Create child_to_child pipe
	int child_to_child[2];
	if (pipe(child_to_child) == -1) {
		const char msg[] = "error: failed to create a 'child_to_child' pipe\n";
		write(STDERR_FILENO, msg, sizeof(msg) - 1);
		exit(EXIT_FAILURE);
	}

	// Create child_to_parent pipe
	int child_to_parent[2];
	if (pipe(child_to_parent) == -1) {
		const char msg[] = "error: failed to create a 'child_to_parent' pipe\n";
		write(STDERR_FILENO, msg, sizeof(msg) - 1);
		exit(EXIT_FAILURE);
	}
	
	// Spawn child1
	const pid_t child_1 = fork();
	if (child_1 == -1) {
		const char msg[] = "error: failed to spawn a new process\n";
		write(STDERR_FILENO, msg, sizeof(msg) - 1);
		exit(EXIT_FAILURE);
	} else if (child_1 == 0) {
		// Close unused pipe ends
		close(parent_to_child[1]);
		close(child_to_child[0]);
		close(child_to_parent[0]);
		close(child_to_parent[1]);

		// Read from parent
		dup2(parent_to_child[0], STDIN_FILENO);
		close(parent_to_child[0]);

		// Write to child2
		dup2(child_to_child[1], STDOUT_FILENO);
		close(child_to_child[1]);

		// Build the full path to child1
		char process_path[PATH_MAX + NAME_MAX + 2];
		snprintf(process_path, sizeof(process_path), "%s/child1", path);
		
		// Execute child1
		execlp(process_path, "child1", NULL);

		// Reached only if exec fails
		const char msg[] = "failed to exec into new executable image\n";
		write(STDERR_FILENO, msg, sizeof(msg) - 1);
		exit(EXIT_FAILURE);
	}

	// Spawn child2
	const pid_t child_2 = fork();
	if (child_2 == -1) {
		const char msg[] = "error: failed to spawn a new process\n";
		write(STDERR_FILENO, msg, sizeof(msg) - 1);
		exit(EXIT_FAILURE);
	} else if (child_2 == 0) {
		// Close unused pipe ends
		close(parent_to_child[0]);
		close(parent_to_child[1]);
		close(child_to_child[1]);
		close(child_to_parent[0]);
		
		// Read from child1
		dup2(child_to_child[0], STDIN_FILENO);
		close(child_to_child[0]);

		// Write to parent
		dup2(child_to_parent[1], STDOUT_FILENO);
		close(child_to_parent[1]);

		// Build full path to child2
		char process_path[PATH_MAX + NAME_MAX + 2];
		snprintf(process_path, sizeof(process_path), "%s/child2", path);

		// Execute child2
		execlp(process_path, "child2", NULL);

		// Reached only if exec fails
		const char msg[] = "failed to exec into new executable image\n";
		write(STDERR_FILENO, msg, sizeof(msg) - 1);
		exit(EXIT_FAILURE);
	}
	
	// Remaining open parent_to_child[1] - to write to child1 and child_to_parent[0] - to read from child2
	close(parent_to_child[0]);
	close(child_to_child[0]);
	close(child_to_child[1]);
	close(child_to_parent[1]);

	// Buffer for reading data from stdin
	char buffer[1024];
	ssize_t bytes = 0;
	ssize_t written = 0;
	
	// User interaction loop. Exit conditions: Ctrl+D (EOF) or an empty line
	while (bytes = read(STDIN_FILENO, buffer, sizeof(buffer))) {
		if (bytes < 0) {
			const char msg[] = "error: failed to read from stdin\n";
			write(STDERR_FILENO, msg, sizeof(msg) - 1);
			exit(EXIT_FAILURE);
		} else if (buffer[0] == '\n') {
			// Empty line - terminate
			break;
		}

		// Send data to child1
		written = write(parent_to_child[1], buffer, bytes);
		if (written != bytes) {
			const char msg[] = "error: failed to write to parent_to_child\n";
            write(STDERR_FILENO, msg, sizeof(msg) - 1);
            exit(EXIT_FAILURE);
		}

		// Read the result from child2
		bytes = read(child_to_parent[0], buffer, sizeof(buffer));
		if (bytes < 0) {
            const char msg[] = "error: failed to read from child_to_parent\n";
            write(STDERR_FILENO, msg, sizeof(msg) - 1);
            exit(EXIT_FAILURE);
        }

		// Print the result for the user
		written = write(STDOUT_FILENO, buffer, bytes);
		if (written != bytes) {
			const char msg[] = "error: failed to write to stdout\n";
            write(STDERR_FILENO, msg, sizeof(msg) - 1);
            exit(EXIT_FAILURE);
		}
	}

	// Close remaining descriptors
	close(parent_to_child[1]);
	close(child_to_parent[0]);

	// wait(NULL) is called twice - waiting for both children
	// Without wait() the children would become zombies (defunct)
	wait(NULL);
	wait(NULL);

	exit(EXIT_SUCCESS);
}