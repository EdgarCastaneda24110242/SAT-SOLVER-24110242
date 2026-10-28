#include "SatApplication.hpp"

#include "AtomTable.h"
#ifdef SAT_HAS_SFML
#include "GuiApp.h"
#endif
#include "Parser.h"
#include "SATSolver.h"

#include <iostream>
#include <string>

namespace {
#ifndef SAT_HAS_SFML
std::string truth(bool value) { return value ? "V" : "F"; }
bool isSymbolic(const std::string& input) {
    return input.find('&') != std::string::npos || input.find('|') != std::string::npos ||
           input.find("->") != std::string::npos || input.find('!') != std::string::npos ||
           input.find('~') != std::string::npos;
}
#endif
}

int runSatApplication(int argc, char** argv) {
#ifdef SAT_HAS_SFML
    return GuiApp().run(argc, argv);
#else
    (void)argc;
    (void)argv;
    std::cout << "SAT-SOLVER-24110242 - Solver proposicional en C++\n";
    std::cout << "Escribe una frase normal o una formula simbolica. Usa 'salir' para terminar.\n\n";

    AtomTable atomTable;
    Parser parser(atomTable);
    SATSolver solver;
    std::string input;
    while (true) {
        std::cout << "Formula> ";
        if (!std::getline(std::cin, input) || input == "salir") break;
        if (input.empty()) continue;
        try {
            const FormulaPtr formula = isSymbolic(input)
                ? parser.parseSymbolic(input)
                : parser.parseNaturalLanguage(input);
            const SATResult result = solver.solve(*formula);
            std::cout << "\nFormula: " << formula->print() << "\nAtomos: ";
            for (std::size_t i = 0; i < result.atoms.size(); ++i)
                std::cout << (i ? ", " : "") << result.atoms[i];
            std::cout << "\nResultado: " << (result.satisfiable ? "SATISFACIBLE" : "NO SATISFACIBLE") << "\n";
            std::cout << "Modelos: " << result.modelCount << "\n";
            for (const auto& model : result.models) {
                std::cout << "  { ";
                for (std::size_t i = 0; i < result.atoms.size(); ++i)
                    std::cout << (i ? ", " : "") << result.atoms[i] << "=" << truth(model.at(result.atoms[i]));
                std::cout << " }\n";
            }
            std::cout << "Asignaciones descartadas: " << result.discardedAssignments << "\n";
            std::cout << "\nArbol semantico de la formula:\n";
            formula->printTree("", true);
            std::cout << "\n";
        } catch (const std::exception& error) {
            std::cout << "Error: " << error.what() << "\n\n";
        }
    }
    return 0;
#endif
}
