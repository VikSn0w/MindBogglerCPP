
#ifndef INTERPRETER_H
#define INTERPRETER_H


#include <vector>
#include <string>
#include <functional>
#include <stdexcept>
#include <memory>

enum class PointerBehavior {
    CLAMP = 0,  // Stay at boundaries
    WRAP = 1,   // Wrap around
    ERROR = 2   // Raise exception
};

enum class CellBehavior {
    WRAP = 0,      // Standard Brainfuck wrap around (0-255)
    UNLIMITED = 1, // Allow values beyond 0-255 range
    ERROR = 2      // Raise exception on underflow/overflow
};

class PointerOverflowError : public std::runtime_error {
public:
    explicit PointerOverflowError(const std::string& message)
        : std::runtime_error(message) {}
};

class CellOverflowError : public std::runtime_error {
public:
    explicit CellOverflowError(const std::string& message)
        : std::runtime_error(message) {}
};

class Interpreter {
    private:
        std::vector<int> memory;
        std::string program;
        std::string outputBuffer;
        std::vector<int> inputBuffer;
        std::vector<std::pair<char, int>> compiledProgram;

        int memorySize;
        int pointer;
        int pc;
        bool running;
        int fastPc;
        int fastSteps;

        PointerBehavior pointerBehavior;
        CellBehavior cellBehavior;
        std::function<std::string()> inputCallback;

        void movePointer(int delta);
        void modifyCell(int delta);

    public:
        explicit Interpreter(int memorySize = 30000);

        void configure(PointerBehavior ptrBehavior, CellBehavior cellBehavior);
        void reset();
        void loadProgram(const std::string& program, const std::string& inputData = "");
        void setInputCallback(std::function<std::string()> callback);

        std::vector<std::pair<int, char>> checkProgramSyntax() const;
        std::string generatePseudocode();
        std::vector<std::pair<char, int>> compileProgram();

        int runProgramFast(int maxSteps = 1000000);
        bool runProgramFastInterruptible(int stepsPerChunk = 10000, int maxSteps = 1000000);
        bool step();
        int runUntilEnd(int maxSteps = 1000000);

        // Getters
        int getPointer() const { return pointer; }
        int getPc() const { return pc; }
        bool isRunning() const { return running; }
        const std::vector<int>& getMemory() const { return memory; }
        const std::string& getOutputBuffer() const { return outputBuffer; }
        int getMemorySize() const { return memorySize; }
        int getFastSteps() const { return fastSteps; }
        PointerBehavior getPointerBehavior() const { return pointerBehavior; }
        CellBehavior getCellBehavior() const { return cellBehavior; }
};


#endif //INTERPRETER_H
