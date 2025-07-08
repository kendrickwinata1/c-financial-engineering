#include "RiskEngine.h"

void RiskEngine::computeRisk(string riskType, shared_ptr<Trade> trade, bool singleThread)
{
	result.clear();
	if (singleThread)
	{
		if (riskType == "dv01")
		{
			for (auto &kv : curveShocks)
			{
				string market_id = kv.first;
				auto mkt_u = kv.second.getMarketUp();
				auto mkt_d = kv.second.getMarketDown();
				double pv_up = trade->Pv(mkt_u);
				double pv_down = trade->Pv(mkt_d);
				double dv01 = (pv_up - pv_down) / 2.0;
				result.emplace(market_id, dv01);
			}
		}

		if (riskType == "vega")
		{
			for (auto &kv : volShocksUp)
			{
				string market_id = kv.first;
				auto mkt_up = kv.second.getMarket();
				auto mkt_down = volShocksDown.at(market_id).getMarket();
				double pv_up = trade->Pv(mkt_up);
				double pv_down = trade->Pv(mkt_down);
				double vega = (pv_up - pv_down) / 2.0;
				result.emplace(market_id, vega);
			}
		}

		if (riskType == "price")
		{
			for (auto &kv : priceShocks)
			{
				string market_id = kv.first;
				auto mkt_orig = kv.second.getOriginMarket();
				auto mkt_bumped = kv.second.getMarket();
				double pv_orig = trade->Pv(mkt_orig);
				double pv_bumped = trade->Pv(mkt_bumped);
				double delta = pv_bumped - pv_orig;
				result.emplace(market_id, delta);
			}
		}
	}
	else
	{
		auto pv_task = [](shared_ptr<Trade> trade, string id, const Market &mkt_up, const Market &mkt_down)
		{
			double pv_up = trade->Pv(mkt_up);
			double pv_down = trade->Pv(mkt_down);
			double risk = (pv_up - pv_down) / 2.0;
			return std::make_pair(id, risk);
		};

		vector<std::future<std::pair<string, double>>> _futures;

		// For IR curve shocks (DV01)
		for (auto &shock : curveShocks)
		{
			string market_id = shock.first;
			auto mkt_u = shock.second.getMarketUp();
			auto mkt_d = shock.second.getMarketDown();
			_futures.push_back(std::async(std::launch::async, pv_task, trade, market_id, mkt_u, mkt_d));
		}

		// For vol shocks (Vega, central diff)
		for (auto &shock : volShocksUp)
		{
			string market_id = shock.first;
			auto mkt_up = shock.second.getMarket();
			auto mkt_down = volShocksDown.at(market_id).getMarket();
			_futures.push_back(std::async(std::launch::async, pv_task, trade, market_id, mkt_up, mkt_down));
		}

		// For price shocks (Delta)
		for (auto &shock : priceShocks)
		{
			string market_id = shock.first;
			auto mkt_orig = shock.second.getOriginMarket();
			auto mkt_bumped = shock.second.getMarket();
			auto price_task = [trade, market_id, mkt_orig, mkt_bumped]()
			{
				double pv_orig = trade->Pv(mkt_orig);
				double pv_bumped = trade->Pv(mkt_bumped);
				double delta = pv_bumped - pv_orig;
				return std::make_pair(market_id, delta);
			};
			_futures.push_back(std::async(std::launch::async, price_task));
		}

		for (auto &&fut : _futures)
		{
			auto rs = fut.get();
			result.emplace(rs);
		}
	}
}
