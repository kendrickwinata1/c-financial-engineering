#include "Bond.h"
#include "Market.h"
#include <cmath>

void Bond::generateSchedule()
{
	// implement this
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
		bondSchedule.push_back(seed);
		seed = dateAddTenor(seed, tenorStr);
	}
	bondSchedule.push_back(maturityDate);
	if (bondSchedule.size() < 2)
		throw std::runtime_error("Error: invalid schedule, check input!");
}
double Bond::Payoff(double s) const
{
	double pv = notional * (s - tradePrice);
	return pv;
}
double Bond::Pv(const Market &mkt) const
{
	// using cash flow discunting
	//  implement this
	double pv = 0.0;
	std::string dir = direction;
	std::transform(dir.begin(), dir.end(), dir.begin(), ::tolower);
	double sign = (dir == "short") ? -1.0 : 1.0;

	auto rc = mkt.getCurve(rateCurve);
	Date valueDate = mkt.asOf;

	// Loop through all coupon payment dates
	for (size_t i = 1; i < bondSchedule.size(); ++i)
	{
		Date dt = bondSchedule[i];
		if (dt < valueDate)
			continue;
		// Year fraction between two coupon dates (e.g., 180/360 for semi-annual)
		double tau = (bondSchedule[i] - bondSchedule[i - 1]) / 360.0;
		// Interpolated discount factor for this coupon date
		double zr = rc->getRate(dt);
		double T = (dt - valueDate) / 360.0;
		double df = exp(-zr * T);
		// Coupon cashflow
		pv += coupon * notional * tau * df;
	}
	// Add notional repayment at maturity (discounted)
	Date dt = maturityDate;
	if (dt >= valueDate)
	{
		double zr = rc->getRate(dt);
		double T = (dt - valueDate) / 360.0;
		double df = exp(-zr * T);
		pv += notional * df;
	}
	return sign * pv;
}
