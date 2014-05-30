#
# Copyright 2014 Charles University in Prague
# Copyright 2014 Vojtech Horky
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
 
OUTPUT_DIRECTORY = ./

JAVAH = javah
JNI_CLASSNAME_BASE = cz.cuni.mff.d3s.perf
JNI_CLASS_DIR = ../java/$(subst .,/,$(JNI_CLASSNAME_BASE))
JNI_C_HEADER_BASE = cz_cuni_mff_d3s_perf

CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -Wno-unused-parameter -Werror -I$(OUTPUT_DIRECTORY)
CFLAGS_JNI = -I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/linux -fPIC

AGENT_SOURCES = counter.c agentmain.c jnifunc.c benchmark.c estimations.c
AGENT_OBJECTS = $(addprefix $(OUTPUT_DIRECTORY), $(addsuffix .o, $(basename $(AGENT_SOURCES))))

AGENT_FILE = ubench-agent.so

all: $(OUTPUT_DIRECTORY)$(AGENT_FILE)

$(OUTPUT_DIRECTORY)$(AGENT_FILE): $(AGENT_OBJECTS)
	$(CC) $(CFLAGS) -o $@ -shared -lrt $(CFLAGS_JNI) $(AGENT_OBJECTS)

$(OUTPUT_DIRECTORY)%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $(CFLAGS_JNI) $<

$(AGENT_SOURCES): $(OUTPUT_DIRECTORY)$(JNI_C_HEADER_BASE)_CompilationCounter.h
$(AGENT_SOURCES): $(OUTPUT_DIRECTORY)$(JNI_C_HEADER_BASE)_Benchmark.h
$(AGENT_SOURCES): $(OUTPUT_DIRECTORY)$(JNI_C_HEADER_BASE)_OverheadEstimations.h

$(OUTPUT_DIRECTORY)$(JNI_C_HEADER_BASE)_%.h: $(JNI_CLASS_DIR)/%.java
	$(JAVAH) -cp ../java -d $(OUTPUT_DIRECTORY) $(JNI_CLASSNAME_BASE).$*

clean:
	rm -rf $(OUTPUT_DIRECTORY)*.so $(OUTPUT_DIRECTORY)*.o $(OUTPUT_DIRECTORY)$(JNI_C_HEADER_BASE)*.h

