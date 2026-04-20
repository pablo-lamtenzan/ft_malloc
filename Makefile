# ==============================================================================
# Build Configuration
# ==============================================================================
NAME        := project_name
TEST_NAME   := test_basic

CC          ?= clang
RM          := rm -rf

OBJDIR      := build/obj
TEST_OBJDIR := build/test_obj

CFLAGS      := -Wall -Wextra -Werror -Iinclude -O2 -fPIC
TEST_CFLAGS := -Wall -Wextra -Werror -Iinclude -O0 -g3
SAN_CFLAGS  := $(TEST_CFLAGS) -fsanitize=address,undefined -fno-omit-frame-pointer
COV_CFLAGS  := $(TEST_CFLAGS) --coverage

LDFLAGS     :=
TEST_LDFLAGS:= -lcriterion
SAN_LDFLAGS := -fsanitize=address,undefined -lcriterion
COV_LDFLAGS := --coverage -lcriterion

# ==============================================================================
# Sources
# ==============================================================================
# Include auto-generated srcs.mk
-include srcs.mk

# Filter out main.c from the library source so tests can link properly
LIB_SRCS    := $(filter-out src/main.c, $(SRCS))
TEST_SRCS   := $(shell find tests -type f -name '*.c' 2>/dev/null)

OBJS        := $(patsubst src/%.c, $(OBJDIR)/%.o, $(SRCS))
LIB_OBJS    := $(patsubst src/%.c, $(TEST_OBJDIR)/src/%.o, $(LIB_SRCS))
TEST_OBJS   := $(patsubst tests/%.c, $(TEST_OBJDIR)/tests/%.o, $(TEST_SRCS))

# ==============================================================================
# Main Rules
# ==============================================================================
.PHONY: all clean fclean re test san coverage format format-check tidy compdb valgrind check

all: $(NAME)

$(NAME): $(OBJS)
	@$(CC) $(CFLAGS) -o $(NAME) $(OBJS) $(LDFLAGS)
	@echo "🔗 Linked executable: $@"

$(OBJDIR)/%.o: src/%.c
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c -o $@ $<
	@echo "🔨 Compiled: $<"

# ==============================================================================
# Testing Rules
# ==============================================================================
$(TEST_NAME): $(LIB_OBJS) $(TEST_OBJS)
	@$(CC) $(TEST_CFLAGS) -o $(TEST_NAME) $(LIB_OBJS) $(TEST_OBJS) $(TEST_LDFLAGS)
	@echo "🔗 Linked test binary: $@"

$(TEST_OBJDIR)/src/%.o: src/%.c
	@mkdir -p $(dir $@)
	@$(CC) $(TEST_CFLAGS) -c -o $@ $<
	@echo "🔨 Compiled (test mode): $<"

$(TEST_OBJDIR)/tests/%.o: tests/%.c
	@mkdir -p $(dir $@)
	@$(CC) $(TEST_CFLAGS) -c -o $@ $<
	@echo "🔨 Compiled test: $<"

test: $(TEST_NAME)
	@echo "🚀 Running tests..."
	@./$(TEST_NAME)

# ==============================================================================
# Analysis and Check Rules
# ==============================================================================
san:
	@echo "🛡️  Building and running with Address & UndefinedBehavior Sanitizers..."
	@$(MAKE) fclean
	@$(MAKE) test TEST_CFLAGS="$(SAN_CFLAGS)" TEST_LDFLAGS="$(SAN_LDFLAGS)"

coverage:
	@echo "📊 Building and running with Coverage enabled..."
	@$(MAKE) fclean
	@$(MAKE) test TEST_CFLAGS="$(COV_CFLAGS)" TEST_LDFLAGS="$(COV_LDFLAGS)"
	@./scripts/coverage.sh

valgrind: test
	@./scripts/run-valgrind.sh ./$(TEST_NAME)

format:
	@echo "🎨 Formatting source files..."
	@find src include tests -type f -name '*.[ch]' -exec clang-format -i {} +
	@echo "✅ Formatting complete."

format-check:
	@echo "🔍 Checking code format..."
	@find src include tests -type f -name '*.[ch]' -exec clang-format --dry-run -Werror {} +
	@echo "✅ Format is perfect."

compdb:
	@./scripts/gen-compdb.sh

tidy: compdb
	@./scripts/run-clang-tidy.sh

check: format-check tidy test valgrind san coverage
	@echo "✨ All checks passed! Your code is pristine."



# ==============================================================================
# Maintenance Rules
# ==============================================================================
clean:
	@$(RM) build/ coverage/
	@find . -name "*.gcda" -o -name "*.gcno" -o -name "*.gcov" -delete
	@echo "🧹 Cleaned objects and build artifacts."

fclean: clean
	@$(RM) $(NAME) $(TEST_NAME) compile_commands.json
	@echo "🧹 Cleaned executables and databases."

re: fclean all

srcs.mk:
	@./scripts/gen_srcs.sh src include
