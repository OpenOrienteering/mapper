#ifndef OPENORIENTEERING_SYMBOL_UNDO_H
#define OPENORIENTEERING_SYMBOL_UNDO_H

#include <undo/undo.h>

namespace OpenOrienteering {

class Map;
class Symbol;

class SymbolUndoStep : public UndoStep
{
public:
    enum SymbolChange {	UndefinedChange, AddSymbol,	RemoveSymbol, ChangeSymbol };

	SymbolUndoStep(Map* map, SymbolChange change_type, Symbol* symbol, Symbol* new_symbol);
	SymbolUndoStep(Map* map, SymbolChange change_type, Symbol* symbol, int pos);
	SymbolUndoStep(Map* map);
	SymbolUndoStep() = delete;

	~SymbolUndoStep() override = default;

	UndoStep* undo() override;
	bool isValid() const override;

protected:
	void saveImpl(QXmlStreamWriter& xml) const;
	void loadImpl(QXmlStreamReader& xml, SymbolDictionary&symbol_dict);

	SymbolChange change_type;
	Symbol* symbol_ptr;
	Symbol* new_symbol_ptr;
	int symbol_pos;
};

}  // namespace OpenOrienteering

#endif // OPENORIENTEERING_SYMBOL_UNDO_H
