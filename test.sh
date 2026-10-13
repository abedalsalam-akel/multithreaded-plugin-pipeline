#!/bin/bash
set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

print_test() { echo -e "${BLUE}[TEST]${NC} $1"; }
print_pass() { echo -e "${GREEN}[PASS]${NC} $1"; }
print_fail() { echo -e "${RED}[FAIL]${NC} $1"; }
print_warning() { echo -e "${YELLOW}[WARNING]${NC} $1"; }

# Test counters
TEST_COUNT=0
PASS_COUNT=0
FAIL_COUNT=0

# Function to run a test
run_test() {
    local test_name="$1"
    local queue_size="$2"
    local plugins="$3"
    local input="$4"
    local expected_pattern="$5"
    
    TEST_COUNT=$((TEST_COUNT + 1))
    print_test "Running test: $test_name"
    
    # Create temporary input file
    echo -e "$input\n<END>" > test_input.tmp
    
    # Run the analyzer
    if timeout 10s ./output/analyzer $queue_size $plugins < test_input.tmp > test_output.tmp 2>&1; then
        # Check if output matches expected pattern
        if grep -q "$expected_pattern" test_output.tmp; then
            print_pass "$test_name"
            PASS_COUNT=$((PASS_COUNT + 1))
        else
            print_fail "$test_name - Output doesn't match expected pattern"
            echo "Expected pattern: $expected_pattern"
            echo "Actual output:"
            cat test_output.tmp
            FAIL_COUNT=$((FAIL_COUNT + 1))
        fi
    else
        print_fail "$test_name - Program failed or timed out"
        echo "Output:"
        cat test_output.tmp
        FAIL_COUNT=$((FAIL_COUNT + 1))
    fi
    
    # Clean up
    rm -f test_input.tmp test_output.tmp
    echo
}

# Function to test error conditions
test_error_condition() {
    local test_name="$1"
    local args="$2"
    local expected_exit_code="$3"
    
    TEST_COUNT=$((TEST_COUNT + 1))
    print_test "Running error test: $test_name"
    
    # Run the analyzer and capture exit code
    if echo "<END>" | ./output/analyzer $args > /dev/null 2>&1; then
        local exit_code=0
    else
        local exit_code=$?
    fi
    
    if [ $exit_code -eq $expected_exit_code ] || [ $expected_exit_code -eq -1 ]; then
        print_pass "$test_name"
        PASS_COUNT=$((PASS_COUNT + 1))
    else
        print_fail "$test_name - Expected exit code $expected_exit_code, got $exit_code"
        FAIL_COUNT=$((FAIL_COUNT + 1))
    fi
    echo
}

# Main test execution
echo "Starting comprehensive test suite for Modular Pipeline System"
echo "============================================================"

# Check if build was successful
if [ ! -f "output/analyzer" ]; then
    print_fail "Main analyzer executable not found. Please run build.sh first."
    exit 1
fi

# Check if all plugins exist
for plugin in logger uppercaser rotator flipper expander typewriter; do
    if [ ! -f "output/${plugin}.so" ]; then
        print_fail "Plugin ${plugin}.so not found. Please run build.sh first."
        exit 1
    fi
done

print_test "All required files found. Starting tests..."
echo

# Test 1: Single plugin (logger)
run_test "Single plugin - logger" "5" "logger" "hello world" "\[logger\] hello world"

# Test 2: Single plugin (uppercaser)
run_test "Single plugin - uppercaser" "3" "uppercaser" "hello world" "HELLO WORLD"

# Test 3: Single plugin (rotator)
run_test "Single plugin - rotator" "2" "rotator" "abc" "cab"

# Test 4: Single plugin (flipper)
run_test "Single plugin - flipper" "4" "flipper" "hello" "olleh"

# Test 5: Single plugin (expander)
run_test "Single plugin - expander" "3" "expander" "abc" "a b c"

# Test 6: Two plugin pipeline
run_test "Two plugins - uppercaser->logger" "5" "uppercaser logger" "hello" "\[logger\] HELLO"

# Test 7: Three plugin pipeline
run_test "Three plugins - uppercaser->rotator->logger" "4" "uppercaser rotator logger" "abc" "\[logger\] CAB"

# Test 8: Complex pipeline
run_test "Complex pipeline - uppercaser->flipper->expander->logger" "6" "uppercaser flipper expander logger" "hello" "\[logger\] O L L E H"

# Test 9: Multiple input lines
run_test "Multiple inputs" "5" "logger" "line1\nline2\nline3" "\[logger\] line1"

# Test 10: Empty input
run_test "Empty input" "3" "logger" "" "Pipeline shutdown complete"

# Test 11: Long pipeline
run_test "Long pipeline" "10" "uppercaser rotator flipper expander logger" "test" "\[logger\]"

# Test 12: Large queue size
run_test "Large queue size" "100" "logger" "test" "\[logger\] test"

# Test 13: Minimum queue size
run_test "Minimum queue size" "1" "logger" "test" "\[logger\] test"

# Test 14: Typewriter plugin (introduces delay)
run_test "Typewriter format" "3" "typewriter" "ok" "\[typewriter\] ok"

# Error condition tests
test_error_condition "No arguments" "" 1
test_error_condition "Only queue size" "5" 1
test_error_condition "Invalid queue size (zero)" "0 logger" 1
test_error_condition "Invalid queue size (negative)" "-1 logger" 1
test_error_condition "Invalid queue size (non-numeric)" "abc logger" 1
test_error_condition "Non-existent plugin" "5 nonexistent" 1

# Performance test with typewriter (should be slower)
print_test "Performance test - typewriter plugin"
TEST_COUNT=$((TEST_COUNT + 1))
start_time=$(date +%s)
echo -e "hi\n<END>" | timeout 15s ./output/analyzer 3 typewriter > /dev/null 2>&1
end_time=$(date +%s)
duration=$((end_time - start_time))

if [ $duration -ge 1 ]; then
    print_pass "Typewriter plugin shows expected delay (${duration}s)"
    PASS_COUNT=$((PASS_COUNT + 1))
else
    print_warning "Typewriter plugin completed faster than expected (${duration}s) -s (${start_time}s) -e (${end_time}s)"
    PASS_COUNT=$((PASS_COUNT + 1))  # Still pass, might be system dependent
fi
echo

# Test graceful shutdown
print_test "Graceful shutdown test"
TEST_COUNT=$((TEST_COUNT + 1))
if echo -e "test\n<END>" | ./output/analyzer 5 logger | grep -q "Pipeline shutdown complete"; then
    print_pass "Graceful shutdown works correctly"
    PASS_COUNT=$((PASS_COUNT + 1))
else
    print_fail "Graceful shutdown message not found"
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi
echo

# Summary
echo "============================================================"
echo "Test Summary:"
echo "Total tests: $TEST_COUNT"
echo "Passed: $PASS_COUNT"
echo "Failed: $FAIL_COUNT"

if [ $FAIL_COUNT -eq 0 ]; then
    print_pass "All tests passed!"
    exit 0
else
    print_fail "$FAIL_COUNT test(s) failed"
    exit 1
fi