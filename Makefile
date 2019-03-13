RM := rm -rf


CROSS_PREFIX ?=
DESTDIR ?= _install
BINDIR ?= /usr/bin


# All Target
all: watchdogd

# Tool invocations
watchdogd: src/watchdog.c
	@echo 'Building target: $@'
	$(CROSS_PREFIX)gcc  -o "watchdogd" src/watchdog.c
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) watchdogd
	-@echo ' '

.PHONY: all clean dependents

install:
	mkdir -p $(DESTDIR)$(BINDIR)
	install -m 744 watchdogd $(DESTDIR)$(BINDIR) 
