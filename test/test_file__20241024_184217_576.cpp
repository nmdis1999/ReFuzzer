
#include <iostream>
#include <vector>
#include <algorithm>
#include <limits>

// Define a struct to represent an operation in the intermediate representation
struct Operation {
    int op;  // Operator (+, -, *, /)
    int operand1;  // Index of operand 1
    int operand2;  // Index of operand 2
};

int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <expression> <output_file>" << std::endl;
        return 1;
    }

    std::string expression = argv[1];
    std::string output_file = argv[2];

    // Parse the expression and generate intermediate representation
    Operation* ir = parseExpression(expression);
    if (ir == nullptr) {
        std::cerr << "Failed to parse expression: " << expression << std::endl;
        return 1;
    }

    // Allocate registers for each operation in the IR
    std::vector<std::pair<int, int>> registerAllocations;

    allocateRegisters(ir, registerAllocations);

    // Print the allocated registers
    for (const auto& allocation : registerAllocations) {
        std::cout << "Register " << allocation.first << " allocated to operand " << allocation.second + 1 << std::endl;
    }

    // Write the allocated registers to the output file
    writeRegisters(output_file, registerAllocations);

    return 0;
}

// Helper function to parse the expression and generate intermediate representation
Operation* parseExpression(const std::string& expression) {
    // This is a very basic parser that assumes the input expression is a simple arithmetic expression (e.g. "2+3")
    size_t pos = 0;
    Operation* ir = nullptr;

    if (expression[pos] == '+') {
        int operand1 = parseOperand(expression, ++pos);
        int op = +1;  // Operator for addition
        int operand2 = parseOperand(expression, pos);  // Parse the next operand

        // Create an operation in the IR
        Operation* newOp = new Operation();
        newOp->op = op;
        newOp->operand1 = operand1;
        newOp->operand2 = operand2;

        if (ir == nullptr) {
            ir = newOp;
        } else {
            ir->next = newOp;  // Add the operation to the end of the IR
        }
    }

    return ir;
}

// Helper function to parse a single operand from the expression
int parseOperand(const std::string& expression, size_t& pos) {
    if (expression[pos] >= '0' && expression[pos] <= '9') {
        int value = 0;
        while (pos < expression.size() && expression[pos] >= '0' && expression[pos] <= '9') {
            value = value * 10 + (expression[pos++] - '0');
        }
        return value;
    } else if (expression[pos] == '-') {
        pos++;
        int value = parseOperand(expression, pos);  // Parse the next operand
        return -value;  // Negate the value
    } else if (expression[pos] == '*') {
        pos++;
        int value = parseOperand(expression, pos);  // Parse the next operand
        return value * 2;
    }

    std::cerr << "Failed to parse operand: " << expression.substr(pos) << std::endl;
    exit(1);
}

// Helper function to allocate registers for each operation in the IR
void allocateRegisters(Operation* ir, std::vector<std::pair<int, int>>& registerAllocations) {
    if (ir == nullptr) return;

    // Allocate a new register for this operation
    int reg = getAvailableRegister();  // Get an available register

    // Store the operand indices in the register
    storeOperandIndices(reg, ir->operand1);  // Store the first operand index
    storeOperandIndices(reg, ir->operand2);  // Store the second operand index

    registerAllocations.push_back({reg, ir->operand1});  // Add to the list of allocated registers

    allocateRegisters(ir->next, registerAllocations);  // Recursively allocate registers for the next operation
}

// Helper function to get an available register
int getAvailableRegister() {
    static std::vector<int> availableRegisters = {0, 1, 2};  // Assume we have three registers

    int reg = availableRegisters.back();
    availableRegisters.pop_back();

    return reg;
}

// Helper function to store the operand indices in a register
void storeOperandIndices(int reg, int operandIndex) {
    // This is a very basic implementation that assumes the input expression uses a limited number of registers
    switch (operandIndex) {
        case 1:
            reg |= 1 << 0;  // Set bit 0 to 1
            break;
        case 2:
            reg |= 1 << 1;  // Set bit 1 to 1
            break;
        default:
            std::cerr << "Invalid operand index: " << operandIndex << std::endl;
            exit(1);
    }
}

// Helper function to write the allocated registers to a file
void writeRegisters(const std::string& output_file, const std::vector<std::pair<int, int>>& registerAllocations) {
    std::ofstream file(output_file);

    if (file.is_open()) {
        for (const auto& allocation : registerAllocations) {
            file << "Register " << allocation.first << " allocated to operand " << allocation.second + 1 << std::endl;
        }
    } else {
        std::cerr << "Unable to open output file: " << output_file << std::endl;
    }

    file.close();
}