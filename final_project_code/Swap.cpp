#include <cmath>
#include "Swap.h"
#include "Market.h"

void Swap::generateSchedule()
{
	if (startDate == maturityDate || frequency <= 0 || frequency > 1)
		throw std::runtime_error("Error: start date is later than end date, or invalid frequency!");

	string tenorStr;
	if (frequency == 0.25)
		tenorStr = "3M";
	else if (frequency == 0.5)
		tenorStr = "6M";
	else
		tenorStr = "1Y";

	Date seed = startDate;
	while (seed < maturityDate)
	{
		swapSchedule.push_back(seed);
		seed = dateAddTenor(seed, tenorStr);
	}
	swapSchedule.push_back(maturityDate);
	if (swapSchedule.size() < 2)
		throw std::runtime_error("Error: invalid schedule, check input!");
}

double Swap::Payoff(double s) const
{
	// this function will not be called
	return (s - tradeRate) * notional;
}

double Swap::getAnnuity(const Market &mkt) const
{
	double annuity = 0;
	Date valueDate = mkt.asOf;
	auto rc = mkt.getCurve(rateCurve);
	for (size_t i = 1; i < swapSchedule.size(); i++)
	{
		auto dt = swapSchedule[i];
		if (dt < valueDate)
			continue;
		double tau = (swapSchedule[i] - swapSchedule[i - 1]) / 360.0;
		// Correct discount factor using zero rate interpolation:
		double zr = rc->getRate(dt);
		double T = (dt - valueDate) / 360.0;
		double df = exp(-zr * T);
		annuity += notional * tau * df;
	}
	return annuity;
}

double Swap::Pv(const Market& mkt) const
{
	// using cash flow discunting
	Date valueDate = mkt.asOf;
	auto rc = mkt.getCurve(rateCurve);
	double pvFix = 0.0;
	double pvFloat = 0.0;

	// Use the absolute notional for calculating the value of each leg
	double absNotional = std::abs(notional);

	// --- 1. Calculate the value of the Fixed Leg ---
	for (size_t i = 1; i < swapSchedule.size(); ++i)
	{
		Date payDate = swapSchedule[i];
		if (payDate < valueDate)
			continue;
		double tau = (swapSchedule[i] - swapSchedule[i - 1]) / 365.0; // Using consistent 365 day count
		double df = rc->getDf(payDate);
		pvFix += absNotional * tradeRate * tau * df;
	}

	// --- 2. Calculate the value of the Floating Leg ---
	if (maturityDate >= valueDate)
	{
		// DF at the start of the cashflow stream.
		double df_start = (startDate < valueDate) ? 1.0 : rc->getDf(startDate);

		// DF at the maturity of the swap.
		double df_maturity = rc->getDf(maturityDate);

		pvFloat = absNotional * (df_start - df_maturity);
	}

	// --- 3. Determine final PV based on trade direction ---
	// If notional > 0, it's a "payer" swap (Pay Fixed, Receive Float).
	// If notional < 0, it's a "receiver" swap (Receive Fixed, Pay Float).
	if (notional > 0) {
		return pvFloat - pvFix;
	}
	return pvFix - pvFloat;
}
