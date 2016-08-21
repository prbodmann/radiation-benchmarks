CC=gcc
OMP=-fopenmp

SOURCES_CHECK=main_check.c kernel/kernel_cpu.c  ../../include/log_helper.c
SOURCES_GEN=main.c kernel/kernel_cpu.c 
APP_NAME=lavamd

all: generate lavamd lavamd_err_inj lavamd_timing

generate: $(SOURCES_GEN)
	$(CC) $(SOURCES_GEN) -lm $(OMP) -o $(APP_NAME)_genGold

lavamd: $(SOURCES_CHECK)
	$(CC) $(SOURCES_CHECK) -lm $(OMP) -DLOGS -o $(APP_NAME)

lavamd_timing: $(SOURCES_CHECK)
	$(CC) $(SOURCES_CHECK) -lm $(OMP) -DTIMING -o $(APP_NAME)_nologs_timing

lavamd_err_inj: $(SOURCES_CHECK)
	$(CC) $(SOURCES_CHECK) -lm $(OMP) -DLOGS -DERR_INJ -o $(APP_NAME)_err_inj

clean:
	rm -f input_* output_* $(APP_NAME)_genGold $(APP_NAME) $(APP_NAME)_nologs_timing $(APP_NAME)_err_inj

