/* TransferFunctionControls.h */
#ifndef LMMS_TRANSFERFUNCTION_CONTROLS_H
#define LMMS_TRANSFERFUNCTION_CONTROLS_H

#include "EffectControls.h"
#include "TransferFunctionControlDialog.h"
#include <QString> // Added

namespace lmms
{

class TransferFunctionEffect;

class TransferFunctionControls : public EffectControls
{
	Q_OBJECT
public:
	TransferFunctionControls(TransferFunctionEffect* effect);
	~TransferFunctionControls() override = default;

	void saveSettings(QDomDocument& doc, QDomElement& parent) override;
	void loadSettings(const QDomElement& parent) override;
	
	// Helper to access the string safely
	QString getFormula() const { return m_formulaString; }

	inline QString nodeName() const override
	{
		return "TransferFunctionControls";
	}
	gui::EffectControlDialog* createView() override
	{
		return new gui::TransferFunctionControlDialog(this);
	}
	int controlCount() override { return 4; }

public slots:
	// Slot to receive text updates from GUI
	void setFormula(const QString& text) { m_formulaString = text; }

private:
	TransferFunctionEffect* m_effect;
	FloatModel m_volumeModel;
	
	// The String Variable
	QString m_formulaString;

	friend class gui::TransferFunctionControlDialog;
	friend class TransferFunctionEffect;
};

} // namespace lmms

#endif // LMMS_TRANSFERFUNCTION_CONTROLS_H
