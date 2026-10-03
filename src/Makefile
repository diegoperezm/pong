# Run from the tests/ directory. Expects config.h and log.c/log.h (your
# unchanged originals) in the project root, one level up.

CC     ?= cc
CFLAGS  = -std=c11 -Wall -Wextra -g -O1 -I..
SRC     = ../app.c ../simulation.c ../entities.c ../events.c ../fx.c \
          ../replay.c ../input_source.c ../checksum.c ../log.c

# Plumbing test against the bundled fake physics (no dependencies).
fake:
	$(CC) $(CFLAGS) -Ifake_box2d test_determinism.c fake_box2d/fake_box2d.c $(SRC) -lm -o test_fake
	./test_fake

# Determinism test against REAL Box2D. Example:
#   make real BOX2D_INC=/path/to/box2d/include BOX2D_LIB=/path/to/libbox2d.a
real:
	$(CC) $(CFLAGS) -I$(BOX2D_INC) test_determinism.c $(SRC) $(BOX2D_LIB) -lm -o test_real
	./test_real

clean:
	rm -f test_fake test_real
