#!/bin/bash
set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

print_status() { echo -e "${GREEN}[BUILD]${NC} $1"; }
print_error() { echo -e "${RED}[ERROR]${NC} $1"; }
print_warning() { echo -e "${YELLOW}[WARNING]${NC} $1"; }

# Create output directory
print_status "Creating output directory"
mkdir -p output

# Build main application
print_status "Building main application"
gcc -Wall -Wextra -std=c99 -o output/analyzer main.c -ldl -lpthread || {
    print_error "Failed to build main application"
    exit 1
}

# Build synchronization libraries first
print_status "Building synchronization libraries"
gcc -Wall -Wextra -std=c99 -fPIC -c plugins/sync/monitor.c -o output/monitor.o || {
    print_error "Failed to build monitor.o"
    exit 1
}

gcc -Wall -Wextra -std=c99 -fPIC -c plugins/sync/consumer_producer.c -o output/consumer_producer.o || {
    print_error "Failed to build consumer_producer.o"
    exit 1
}

# Build plugin common infrastructure
print_status "Building plugin common infrastructure"
gcc -Wall -Wextra -std=c99 -fPIC -c plugins/plugin_common.c -o output/plugin_common.o || {
    print_error "Failed to build plugin_common.o"
    exit 1
}

# Build plugins
for plugin_name in logger uppercaser rotator flipper expander typewriter; do
    print_status "Building plugin: $plugin_name"
    gcc -Wall -Wextra -std=c99 -fPIC -shared -o output/${plugin_name}.so \
        plugins/${plugin_name}.c \
        output/plugin_common.o \
        output/monitor.o \
        output/consumer_producer.o \
        -lpthread || {
        print_error "Failed to build $plugin_name"
        exit 1
    }
done

# Clean up object files
print_status "Cleaning up object files"
rm -f output/*.o

print_status "Build complete"
print_status "Built files:"
ls -la output/
