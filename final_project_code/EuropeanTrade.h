#ifndef _EUROPEAN_TRADE
#define _EUROPEAN_TRADE

#include <cassert>
#include "TreeProduct.h"
#include "Payoff.h"
#include "Types.h"
#include "Pricer.h"
#include <cmath>

class EuropeanOption : public TreeProduct
{
public:
	EuropeanOption() {};
	EuropeanOption(OptionType _optType, double _notional, double _strike, const Date &_start, const Date &_expiry, const std::string &name)
	{
		tradeType = "TreeProduct";
		underlying = to_upper(name);
		optType = _optType;
		strike = _strike;
		expiryDate = _expiry;
		notional = _notional;
		tradeDate = _start;
		rateCurve = "USD-SOFR"; // default rate curve, can be changed later
	};
	inline string getType() const override { return tradeType; };
	inline string getUnderlying() const override { return underlying; };
	inline double getNotional() const override { return notional; }
	virtual double Payoff(double S) const override { return PAYOFF::VanillaOption(optType, strike, S); }
	virtual const Date &GetExpiry() const override { return expiryDate; }
	virtual double ValueAtNode(double S, double t, double continuation) const override { return continuation; }

	virtual double Pv(const Market &mkt) const override
	{
		// Use CRR binomial tree model, 50 steps
		CRRBinomialTreePricer pricer(50);
		return pricer.Price(mkt, std::make_shared<EuropeanOption>(*this));
	}

	// Optional: Black-Scholes price for comparison
	virtual double BlackPv(const Market &mkt) const
	{
		double S = mkt.getStockPrice(underlying); // spot
		double K = strike;
		double T = (expiryDate - mkt.asOf) / 360.0;
		auto rc = mkt.getCurve(rateCurve);
		double r = rc->getRate(expiryDate);
		auto vc = mkt.getVolCurve("LOGVOL");
		double vol = vc->getVol(expiryDate);

		if (T <= 0 || vol <= 0)
			return 0.0;

		double d1 = (std::log(S / K) + (r + 0.5 * vol * vol) * T) / (vol * std::sqrt(T));
		double d2 = d1 - vol * std::sqrt(T);

		auto N = [](double x)
		{ return 0.5 * std::erfc(-x / std::sqrt(2)); };

		double df = std::exp(-r * T);
		double price = 0.0;
		if (optType == OptionType::Call)
			price = S * N(d1) - K * df * N(d2);
		else
			price = K * df * N(-d2) - S * N(-d1);

		return price * notional;
	}

protected:
	OptionType optType;
	double strike = 0;
	Date expiryDate;
	string rateCurve;
};

class EuroCallSpread : public EuropeanOption
{
public:
	EuroCallSpread(double _k1, double _k2, const Date &_expiry) : strike1(_k1), strike2(_k2)
	{
		expiryDate = _expiry;
		assert(_k1 < _k2);
	};
	virtual double Payoff(double S) const { return PAYOFF::CallSpread(strike1, strike2, S); };
	virtual const Date &GetExpiry() const { return expiryDate; };

private:
	double strike1;
	double strike2;
};

#endif
