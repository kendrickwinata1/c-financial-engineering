#pragma once
#include <iostream>
#include <string>
#include <map>
#include <unordered_map>
#include <future>
#include <memory>

#include "Trade.h"
#include "Market.h"

using namespace std;

struct MarketShock
{
	string market_id;
	pair<Date, double> shock; // tenor and value
};

// --- Curve Decorator ---
class CurveDecorator : public Market
{
public:
	CurveDecorator(const Market &mkt, const MarketShock &curveShock)
		: thisMarketUp(mkt), thisMarketDown(mkt)
	{
		cout << "curve decorator is created" << endl;
		auto curve_up = thisMarketUp.getCurve(curveShock.market_id);
		curve_up->shock(curveShock.shock.first, curveShock.shock.second);
		std::cout << "curve tenor " << curveShock.shock.first << "is shocked " << curveShock.shock.second << endl;

		auto curve_down = thisMarketDown.getCurve(curveShock.market_id);
		curve_down->shock(curveShock.shock.first, -1 * curveShock.shock.second);
		std::cout << "curve tenor " << curveShock.shock.first << "is shocked " << curveShock.shock.second << endl;
	}
	inline const Market &getMarketUp() const { return thisMarketUp; }
	inline const Market &getMarketDown() const { return thisMarketDown; }

private:
	Market thisMarketUp;
	Market thisMarketDown;
};

// --- Vol Decorator ---
class VolDecorator : public Market
{
public:
	VolDecorator(const Market &mkt, const MarketShock &volShock) : originMarket(mkt), thisMarket(mkt)
	{
		cout << "vol decorator is created" << endl;
		auto curve = thisMarket.getVolCurve(volShock.market_id);
		curve->shock(volShock.shock.first, volShock.shock.second);
		cout << "vol curve " << volShock.shock.first << "is shocked" << volShock.shock.second << endl;
	}
	inline const Market &getOriginMarket() const { return originMarket; }
	inline const Market &getMarket() const { return thisMarket; }

private:
	Market originMarket;
	Market thisMarket;
};

// --- Price Decorator ---
class PriceDecorator : public Market
{
public:
	PriceDecorator(const Market &mkt, const MarketShock &priceShock) : originMarket(mkt), thisMarket(mkt)
	{
		cout << "stock price decorator is created" << endl;
		thisMarket.shockPrice(priceShock.market_id, priceShock.shock.second);
	}

	inline const Market &getOriginMarket() const { return originMarket; }
	inline const Market &getMarket() const { return thisMarket; }

private:
	Market originMarket;
	Market thisMarket;
};

class RiskEngine
{
public:
	RiskEngine(const Market &market, double curve_shock, double vol_shock, double price_shock)
	{
		// --- AMENDED: Correct shocks for each curve ---
		auto usdCurveShock = MarketShock();
		usdCurveShock.market_id = "USD-SOFR";
		usdCurveShock.shock = make_pair(Date(), curve_shock);
		auto usdShockedCurve = CurveDecorator(market, usdCurveShock);
		curveShocks.emplace("USD-SOFR", usdShockedCurve);

		auto sgdCurveShock = MarketShock();
		sgdCurveShock.market_id = "SGD-SORA";
		sgdCurveShock.shock = make_pair(Date(), curve_shock);
		auto sgdShockedCurve = CurveDecorator(market, sgdCurveShock);
		curveShocks.emplace("SGD-SORA", sgdShockedCurve);

		// --- Vol shocks (central difference, up/down) ---
		auto volShock = MarketShock();
		volShock.market_id = "LOGVOL";
		volShock.shock = make_pair(Date(), vol_shock);
		auto shockedVol = VolDecorator(market, volShock);
		volShocks.emplace("LOGVOL", shockedVol);

		// Central diff: Vega Up/Down
		auto volShockUp = MarketShock();
		volShockUp.market_id = "LOGVOL";
		volShockUp.shock = make_pair(Date(), +vol_shock);
		auto shockedVolUp = VolDecorator(market, volShockUp);

		auto volShockDown = MarketShock();
		volShockDown.market_id = "LOGVOL";
		volShockDown.shock = make_pair(Date(), -vol_shock);
		auto shockedVolDown = VolDecorator(market, volShockDown);

		volShocksUp.emplace("LOGVOL", shockedVolUp);
		volShocksDown.emplace("LOGVOL", shockedVolDown);

		// Example: price shock on a stock. Not always used, but included for completeness.
		auto priceShockStruct = MarketShock();
		priceShockStruct.market_id = "APPL";
		priceShockStruct.shock = make_pair(Date(), price_shock);
		auto shockedPrice = PriceDecorator(market, priceShockStruct);
		priceShocks.emplace("APPL", shockedPrice);

		cout << " risk engine is created .. " << endl;
	};

	void computeRisk(string riskType, std::shared_ptr<Trade> trade, bool singleThread);

	inline map<string, double> getResult() const
	{
		cout << " risk result: " << endl;
		return result;
	};

private:
	unordered_map<string, CurveDecorator> curveShocks; // e.g. USD-SOFR, SGD-SORA
	unordered_map<string, VolDecorator> volShocks;	   // e.g. LOGVOL
	unordered_map<string, PriceDecorator> priceShocks; // e.g. APPL (or any equity ticker)
	unordered_map<string, VolDecorator> volShocksUp;
	unordered_map<string, VolDecorator> volShocksDown;

	map<string, double> result;
};
