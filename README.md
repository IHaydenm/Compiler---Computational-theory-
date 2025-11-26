# MiniC Compiler — Lexer, Parser, Semantic Analyzer

This project implements a compact but fully functional **compiler for a C-like educational language**.  
It performs:

- **Lexical Analysis**  
- **Syntactic Analysis (Recursive‑Descent Parser)**  
- **Semantic Analysis (Types, Scopes, Symbol Table)**  

The entire compiler is written in **C99**, with no external dependencies.

---

# Features

# Lexical Analyzer
- Identifies identifiers, keywords, literals, operators, punctuation
- Supports comments (`//` and `/* ... */`)
- Handles numeric, boolean, and string literals
- Outputs clean token stream for the parser

# Parser (Recursive Descent)
Supports:
- Arithmetic expressions  
- Logical expressions  
- Assignment  
- Variable declarations  
- Blocks `{ ... }`  
- Control flow:
  - `if`, `elif`, `else`
  - `while`
  - `for`

# Semantic Analyzer
- Symbol table with scope levels  
- Type checking (int, float, bool)  
- Assignment compatibility rules  
- Semantic errors:
  - Redeclarations  
  - Undeclared variable use  
  - Type mismatches  
  - Invalid conditions  

---

# Language Specification

# Data Types
---
- int
- float
- bool
- void
- double
---

### Keywords
---
 - if, elif, else, while, for, true, false, struct
---

### Operators
---
- **+  -  *  /  %**
- **&&  ||  !**
- **<  <=  >  >=  ==  !=**
- **=  ++  --**
---

### Statements
---
- Declaration:    int x = 5;
- Assignment:     x = x + 1;
- If:             if (x < 5) { ... }
- While:          while (x > 0) { ... }
- For:            for (int i = 0; i < 10; i = i+1) { ... }
- Block:          { ... }
- Expr stmt:      x + 2;
---

---

# Project Structure

---
- new.c                 → Main compiler source
- tests/                → Input test programs
- README.md             → This documentation
---

---

# Build Instructions

Compile the compiler using GCC:

---
- Command prompt
- gcc -std=c99 new.c -o compiler
---

---

# Running the Compiler

Run on any source file:

---
- Command prompt
- ./compiler input.txt
---

This automatically:
- Runs lexer  
- Runs parser  
- Runs semantic checks  

---

# Example Input

---
- ** int x = 5;
- while (x < 10) {
-     x = x + 1;
- } **
---

---

# Error Handling

# Lexical errors
- Invalid characters  
- Unterminated strings  
- Bad number formats  

# Syntactic errors
- Missing parentheses  
- Unexpected tokens  

# Semantic errors
- Undeclared variables  
- Redeclaration in same scope  
- Type mismatches  
- Invalid boolean conditions  

---

# Development Process

# Version Control Discipline
- `main` → stable releases  
- `dev` → ongoing development  
- Feature branches:
  - `feature/lexer`
  - `feature/parser`
  - `feature/semantic`

# Tags
- `v0.1` — Lexer complete  
- `v0.2` — Parser integrated  
- `v1.0` — Full semantic analysis  

# Notable Commits
- Added symbol table & scopes  
- Implemented recursive-descent grammar  
- Improved error reporting  

---

# Team Workflow

# Roles (example)
- **Build/CI** – maintained build scripts  
- **Lexer/Parser** – implemented front-end  
- **Semantics** – type checking & symbol table  
- **Documentation/Tests** – created tests and documentation

# Cadence
- Weekly milestone planning  
- Issue board tracked:
  - “Implement relational ops”
  - “Semantic: unify numeric types”
  - “Fix string literal escapes”

---

# Limitations / Future Work
- No code generation backend  
- Structs parsed but not added to symbol table  
- No arrays or pointers  
- Single‑file compilation only

---
