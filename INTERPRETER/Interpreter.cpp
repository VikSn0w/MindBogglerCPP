#include "Interpreter.h"
#include <algorithm>
#include <stack>
#include <sstream>
#include <iostream>
#include <set>

Interpreter::Interpreter(int memorySize)
    : memorySize(memorySize), pointer(0), pc(0), running(false),
      fastPc(0), fastSteps(0),
      pointerBehavior(PointerBehavior::CLAMP),
      cellBehavior(CellBehavior::WRAP) {
    reset();
}

void Interpreter::configure(PointerBehavior ptrBehavior, CellBehavior cellBehavior) {
    this->pointerBehavior = ptrBehavior;
    this->cellBehavior = cellBehavior;
}

void Interpreter::reset() {
    pointer = 0;
    memory.assign(memorySize, 0);
    program.clear();
    pc = 0;
    outputBuffer.clear();
    inputBuffer.clear();
    running = false;
    compiledProgram.clear();
    fastPc = 0;
    fastSteps = 0;
}

void Interpreter::loadProgram(const std::string& program, const std::string& inputData) {
    this->program = program;
    pc = 0;
    outputBuffer.clear();
    inputBuffer.clear();

    for (char c : inputData) {
        inputBuffer.push_back(static_cast<int>(c));
    }

    running = true;
    compiledProgram.clear();
}

void Interpreter::setInputCallback(std::function<std::string()> callback) {
    inputCallback = callback;
}

void Interpreter::movePointer(int delta) {
    int newPointer = pointer + delta;

    switch (pointerBehavior) {
        case PointerBehavior::CLAMP:
            pointer = std::max(0, std::min(newPointer, memorySize - 1));
            break;

        case PointerBehavior::WRAP:
            pointer = ((newPointer % memorySize) + memorySize) % memorySize;
            break;

        case PointerBehavior::ERROR:
            if (newPointer < 0) {
                throw PointerOverflowError("Pointer underflow: attempted to move to " + std::to_string(newPointer));
            } else if (newPointer >= memorySize) {
                throw PointerOverflowError("Pointer overflow: attempted to move to " + std::to_string(newPointer) +
                                         " (max: " + std::to_string(memorySize - 1) + ")");
            } else {
                pointer = newPointer;
            }
            break;
    }
}

void Interpreter::modifyCell(int delta) {
    int newValue = memory[pointer] + delta;

    switch (cellBehavior) {
        case CellBehavior::WRAP:
            memory[pointer] = ((newValue % 256) + 256) % 256;
            break;

        case CellBehavior::UNLIMITED:
            memory[pointer] = newValue;
            break;

        case CellBehavior::ERROR:
            if (newValue < 0) {
                throw CellOverflowError("Cell underflow: attempted to set cell " + std::to_string(pointer) +
                                      " to " + std::to_string(newValue));
            } else if (newValue > 255) {
                throw CellOverflowError("Cell overflow: attempted to set cell " + std::to_string(pointer) +
                                      " to " + std::to_string(newValue));
            } else {
                memory[pointer] = newValue;
            }
            break;
    }
}

std::vector<std::pair<int, char>> Interpreter::checkProgramSyntax() const {
    std::set<char> allowed = {'[', ']', '.', ',', '<', '>', '+', '-'};
    std::vector<std::pair<int, char>> errors;

    for (size_t i = 0; i < program.size(); ++i) {
        if (allowed.find(program[i]) == allowed.end()) {
            errors.emplace_back(i, program[i]);
        }
    }

    return errors;
}

std::string Interpreter::generatePseudocode() {
    std::ostringstream pseudocode;
    pseudocode << "Program loaded with " << program.size() << " characters.\n";
    pseudocode << "Memory initialized with " << memory.size() << " cells.\n";
    pseudocode << "Pointer initialized at position " << pointer << ".\n";
    pseudocode << "pointer = " << pointer << "\n\n";

    std::string behaviorName;
    switch (cellBehavior) {
        case CellBehavior::WRAP: behaviorName = "wrap around (0-255)"; break;
        case CellBehavior::UNLIMITED: behaviorName = "unlimited range"; break;
        case CellBehavior::ERROR: behaviorName = "error on overflow/underflow"; break;
    }
    pseudocode << "Cell behavior: " << behaviorName << "\n\n";

    int originalPointer = pointer;
    pointer = 0;
    int pc = 0;
    std::string tabber;

    while (pc < static_cast<int>(program.size())) {
        char ch = program[pc];

        switch (ch) {
            case '>':
                pointer++;
                pseudocode << tabber << "pointer++ (" << pointer << ")\n";
                break;
            case '<':
                pointer--;
                pseudocode << tabber << "pointer-- (" << pointer << ")\n";
                break;
            case '+':
                if (cellBehavior == CellBehavior::WRAP) {
                    pseudocode << tabber << "memory[pointer] += 1 (mod 256)\n";
                } else if (cellBehavior == CellBehavior::UNLIMITED) {
                    pseudocode << tabber << "memory[pointer] += 1 (unlimited)\n";
                } else {
                    pseudocode << tabber << "memory[pointer] += 1 (0-255, error on overflow)\n";
                }
                break;
            case '-':
                if (cellBehavior == CellBehavior::WRAP) {
                    pseudocode << tabber << "memory[pointer] -= 1 (mod 256)\n";
                } else if (cellBehavior == CellBehavior::UNLIMITED) {
                    pseudocode << tabber << "memory[pointer] -= 1 (unlimited)\n";
                } else {
                    pseudocode << tabber << "memory[pointer] -= 1 (0-255, error on underflow)\n";
                }
                break;
            case '.':
                pseudocode << tabber << "print(char(memory[pointer]))\n";
                break;
            case ',':
                pseudocode << tabber << "memory[pointer] = input_char()\n";
                break;
            case '[':
                pseudocode << tabber << "while memory[pointer] != 0:\n";
                tabber += "  ";
                break;
            case ']':
                if (tabber.size() >= 2) {
                    tabber = tabber.substr(0, tabber.size() - 2);
                }
                pseudocode << tabber << "end while\n";
                break;
        }
        pc++;
    }

    pointer = originalPointer;
    return pseudocode.str();
}

std::vector<std::pair<char, int>> Interpreter::compileProgram() {
    if (program.empty()) {
        throw std::runtime_error("No program loaded to compile.");
    }

    auto errors = checkProgramSyntax();
    if (!errors.empty()) {
        std::ostringstream oss;
        oss << "Syntax errors found: ";
        for (const auto& error : errors) {
            oss << "(" << error.first << ", '" << error.second << "') ";
        }
        throw std::runtime_error(oss.str());
    }

    std::stack<int> stack;
    compiledProgram.clear();
    int pc = 0;
    int length = static_cast<int>(program.size());

    while (pc < length) {
        char cmd = program[pc];

        if (cmd == '[') {
            stack.push(static_cast<int>(compiledProgram.size()));
            compiledProgram.emplace_back('[', -1);
        } else if (cmd == ']') {
            if (stack.empty()) {
                throw std::runtime_error("Unmatched ']' found.");
            }
            int startIdx = stack.top();
            stack.pop();
            int endIdx = static_cast<int>(compiledProgram.size());
            compiledProgram.emplace_back(']', startIdx);
            compiledProgram[startIdx].second = endIdx;
        } else if (cmd == '>' || cmd == '<' || cmd == '+' || cmd == '-') {

            int count = 1;
            while (pc + 1 < length && program[pc + 1] == cmd) {
                count++;
                pc++;
            }
            compiledProgram.emplace_back(cmd, count);
        } else if (cmd == '.' || cmd == ',') {
            compiledProgram.emplace_back(cmd, 0);
        }
        pc++;
    }

    if (!stack.empty()) {
        throw std::runtime_error("Unmatched '[' found.");
    }

    return compiledProgram;
}

int Interpreter::runProgramFast(int maxSteps) {
    if (compiledProgram.empty()) {
        compileProgram();
    }

    int pc = 0;
    int steps = 0;

    while (pc < static_cast<int>(compiledProgram.size()) && steps < maxSteps) {
        const auto& [cmd, arg] = compiledProgram[pc];

        try {
            switch (cmd) {
                case '>':
                    movePointer(arg);
                    break;
                case '<':
                    movePointer(-arg);
                    break;
                case '+':
                    modifyCell(arg);
                    break;
                case '-':
                    modifyCell(-arg);
                    break;
                case '.': {
                    int cellValue = memory[pointer];
                    if (cellBehavior == CellBehavior::UNLIMITED && (cellValue < 0 || cellValue > 255)) {
                        char outputChar = static_cast<char>(std::max(0, std::min(255, cellValue)));
                        outputBuffer += outputChar;
                    } else {
                        outputBuffer += static_cast<char>(cellValue % 256);
                    }
                    break;
                }
                case ',':
                    if (!inputBuffer.empty()) {
                        int inputValue = inputBuffer.front();
                        inputBuffer.erase(inputBuffer.begin());
                        if (cellBehavior == CellBehavior::ERROR && (inputValue < 0 || inputValue > 255)) {
                            throw CellOverflowError("Input value " + std::to_string(inputValue) + " out of range (0-255)");
                        }
                        memory[pointer] = inputValue;
                    } else if (inputCallback) {
                        std::string inputData = inputCallback();
                        if (!inputData.empty()) {
                            for (char c : inputData) {
                                inputBuffer.push_back(static_cast<int>(c));
                            }
                            if (!inputBuffer.empty()) {
                                int inputValue = inputBuffer.front();
                                inputBuffer.erase(inputBuffer.begin());
                                if (cellBehavior == CellBehavior::ERROR && (inputValue < 0 || inputValue > 255)) {
                                    throw CellOverflowError("Input value " + std::to_string(inputValue) + " out of range (0-255)");
                                }
                                memory[pointer] = inputValue;
                            } else {
                                memory[pointer] = 0;
                            }
                        } else {
                            memory[pointer] = 0;
                        }
                    } else {
                        memory[pointer] = 0;
                    }
                    break;
                case '[':
                    if (memory[pointer] == 0) {
                        pc = arg;
                    }
                    break;
                case ']':
                    if (memory[pointer] != 0) {
                        pc = arg;
                    }
                    break;
            }
        } catch (const PointerOverflowError&) {
            throw;
        } catch (const CellOverflowError&) {
            throw;
        }

        pc++;
        steps++;
    }

    running = false;
    return steps;
}

bool Interpreter::runProgramFastInterruptible(int stepsPerChunk, int maxSteps) {
    if (compiledProgram.empty()) {
        compileProgram();
    }

    int chunkSteps = 0;
    while (fastPc < static_cast<int>(compiledProgram.size()) &&
           chunkSteps < stepsPerChunk &&
           fastSteps < maxSteps) {

        const auto& [cmd, arg] = compiledProgram[fastPc];

        try {
            switch (cmd) {
                case '>':
                    movePointer(arg);
                    break;
                case '<':
                    movePointer(-arg);
                    break;
                case '+':
                    modifyCell(arg);
                    break;
                case '-':
                    modifyCell(-arg);
                    break;
                case '.': {
                    int cellValue = memory[pointer];
                    if (cellBehavior == CellBehavior::UNLIMITED && (cellValue < 0 || cellValue > 255)) {
                        char outputChar = static_cast<char>(std::max(0, std::min(255, cellValue)));
                        outputBuffer += outputChar;
                    } else {
                        outputBuffer += static_cast<char>(cellValue % 256);
                    }
                    break;
                }
                case ',':
                    if (!inputBuffer.empty()) {
                        int inputValue = inputBuffer.front();
                        inputBuffer.erase(inputBuffer.begin());
                        if (cellBehavior == CellBehavior::ERROR && (inputValue < 0 || inputValue > 255)) {
                            throw CellOverflowError("Input value " + std::to_string(inputValue) + " out of range (0-255)");
                        }
                        memory[pointer] = inputValue;
                    } else if (inputCallback) {
                        std::string inputData = inputCallback();
                        if (!inputData.empty()) {
                            for (char c : inputData) {
                                inputBuffer.push_back(static_cast<int>(c));
                            }
                            if (!inputBuffer.empty()) {
                                int inputValue = inputBuffer.front();
                                inputBuffer.erase(inputBuffer.begin());
                                if (cellBehavior == CellBehavior::ERROR && (inputValue < 0 || inputValue > 255)) {
                                    throw CellOverflowError("Input value " + std::to_string(inputValue) + " out of range (0-255)");
                                }
                                memory[pointer] = inputValue;
                            } else {
                                memory[pointer] = 0;
                            }
                        } else {
                            memory[pointer] = 0;
                        }
                    } else {
                        memory[pointer] = 0;
                    }
                    break;
                case '[':
                    if (memory[pointer] == 0) {
                        fastPc = arg;
                    }
                    break;
                case ']':
                    if (memory[pointer] != 0) {
                        fastPc = arg;
                    }
                    break;
            }
        } catch (const PointerOverflowError&) {
            throw;
        } catch (const CellOverflowError&) {
            throw;
        }

        fastPc++;
        fastSteps++;
        chunkSteps++;
    }

    if (fastPc >= static_cast<int>(compiledProgram.size()) || fastSteps >= maxSteps) {
        running = false;
        fastPc = 0;
        fastSteps = 0;
        return false;
    }

    return true;
}

bool Interpreter::step() {
    if (!running || pc >= static_cast<int>(program.size())) {
        running = false;
        return false;
    }

    char ch = program[pc];

    try {
        switch (ch) {
            case '>':
                movePointer(1);
                break;
            case '<':
                movePointer(-1);
                break;
            case '+':
                modifyCell(1);
                break;
            case '-':
                modifyCell(-1);
                break;
            case '.': {
                int cellValue = memory[pointer];
                if (cellBehavior == CellBehavior::UNLIMITED && (cellValue < 0 || cellValue > 255)) {
                    char outputChar = static_cast<char>(std::max(0, std::min(255, cellValue)));
                    outputBuffer += outputChar;
                } else {
                    outputBuffer += static_cast<char>(cellValue % 256);
                }
                break;
            }
            case ',':
                if (!inputBuffer.empty()) {
                    int inputValue = inputBuffer.front();
                    inputBuffer.erase(inputBuffer.begin());
                    if (cellBehavior == CellBehavior::ERROR && (inputValue < 0 || inputValue > 255)) {
                        throw CellOverflowError("Input value " + std::to_string(inputValue) + " out of range (0-255)");
                    }
                    memory[pointer] = inputValue;
                } else if (inputCallback) {
                    std::string inputData = inputCallback();
                    if (!inputData.empty()) {
                        for (char c : inputData) {
                            inputBuffer.push_back(static_cast<int>(c));
                        }
                        if (!inputBuffer.empty()) {
                            int inputValue = inputBuffer.front();
                            inputBuffer.erase(inputBuffer.begin());
                            if (cellBehavior == CellBehavior::ERROR && (inputValue < 0 || inputValue > 255)) {
                                throw CellOverflowError("Input value " + std::to_string(inputValue) + " out of range (0-255)");
                            }
                            memory[pointer] = inputValue;
                        } else {
                            memory[pointer] = 0;
                        }
                    } else {
                        memory[pointer] = 0;
                    }
                } else {
                    memory[pointer] = 0;
                }
                break;
            case '[':
                if (memory[pointer] == 0) {
                    int depth = 1;
                    while (depth > 0) {
                        pc++;
                        if (pc >= static_cast<int>(program.size())) {
                            throw std::runtime_error("Unmatched '[' found.");
                        }
                        if (program[pc] == '[') {
                            depth++;
                        } else if (program[pc] == ']') {
                            depth--;
                        }
                    }
                }
                break;
            case ']':
                if (memory[pointer] != 0) {
                    int depth = 1;
                    while (depth > 0) {
                        pc--;
                        if (pc < 0) {
                            throw std::runtime_error("Unmatched ']' found.");
                        }
                        if (program[pc] == ']') {
                            depth++;
                        } else if (program[pc] == '[') {
                            depth--;
                        }
                    }
                }
                break;
        }
    } catch (const PointerOverflowError&) {
        throw;
    } catch (const CellOverflowError&) {
        throw;
    }

    pc++;
    return true;
}

int Interpreter::runUntilEnd(int maxSteps) {
    int steps = 0;
    while (running && steps < maxSteps) {
        if (!step()) {
            break;
        }
        steps++;
    }
    return steps;
}