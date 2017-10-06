/*
 * Copyright 2017 Charles University in Prague
 * Copyright 2017 Vojtech Horky
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#pragma warning(push, 0)
#include <stdio.h>
#include <stdlib.h>
#ifdef __unix__
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#endif
#pragma warning(pop)

#define BARRIER_NAME_PREFIX "java-ubench-agent"

static void die(const char *message) {
	perror(message);
	exit(1);
}

int main(int argc, char *argv[]) {
#ifdef __unix__
	if (argc != 3) {
		fprintf(stderr, "Usage: %s barrier-name barrier-size\n", argv[0]);
		return 1;
	}

	const char *barrier_name_suffix = argv[1];
	unsigned int barrier_size = atoi(argv[2]);

	char *barrier_name = malloc(strlen(barrier_name_suffix) + strlen(BARRIER_NAME_PREFIX) + 1);
	strcpy(barrier_name, BARRIER_NAME_PREFIX);
	strcat(barrier_name, barrier_name_suffix);

	int shared_mem_fd = shm_open(barrier_name, O_RDWR | O_CREAT, 0600);
	if (shared_mem_fd == -1) {
		die("shm_open");
	}

	int err = ftruncate(shared_mem_fd, sizeof(pthread_barrier_t));
	if (err == -1) {
		die("ftruncate");
	}

	pthread_barrier_t *barrier = (pthread_barrier_t*) mmap(0, sizeof(pthread_barrier_t), PROT_READ | PROT_WRITE, MAP_SHARED, shared_mem_fd, 0);
	if (barrier == MAP_FAILED) {
		die("mmap of shared memory");
	}

	pthread_barrierattr_t attr;
	pthread_barrierattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
	err = pthread_barrier_init(barrier, &attr, barrier_size);
	if (err != 0) {
		die("pthread_barrier_init");
	}
#else
	fprintf(stderr, "This program must be run on a Unix system.")
#endif

	return 0;
}
