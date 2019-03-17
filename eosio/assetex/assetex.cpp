#include <string>

#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/transaction.hpp>

using namespace eosio;

/**
* Create assets, exchange assets among accounts
*/
class [[eosio::contract("assetex")]] assetex : public contract {

	public:
		using contract::contract;

		assetex(name self, name code, datastream<const char*> ds):contract(self, code, ds) {}

		// EOSIO Actions

		// Define new configuration for a new asset
		// @abi action
		[[eosio::action]]
		void create(const name& issuer, const asset& max_supply) {

			require_auth(_self);

			auto asset_symbol = max_supply.symbol;
		    eosio_assert(asset_symbol.is_valid(), "invalid symbol name" );
		    eosio_assert(max_supply.is_valid(), "invalid supply");
		    eosio_assert(max_supply.amount > 0, "max-supply must be positive");

		    stats statsdata(_self, _self.value);
		    auto existing = statsdata.find(asset_symbol.raw());
		    eosio_assert(existing == statsdata.end(), "token with symbol already exists" );

		    // create a new asset
		    statsdata.emplace(_self, [&](auto& s ) {
		       s.supply.symbol = max_supply.symbol;
		       s.max_supply    = max_supply;
		       s.issuer        = issuer;
		    });
		}

		// Increase the supply of an asset
		// Issues a specified amount of an asset to a named account and then
		// increments the total supply of the asset 
		// @abi action
		[[eosio::action]]
		void issue(const name& to, const asset& quantity, const std::string& memo) {

			auto asset_symbol = quantity.symbol;
		    eosio_assert(asset_symbol.is_valid(), "invalid symbol name");
		    eosio_assert(memo.size() <= 256, "memo has more than 256 bytes");

		    auto asset_symbol_name = asset_symbol.raw();
		    // code --> _self --> contract
		    // scope --> asset_symbol_name
		    stats statsdata(_self, _self.value);
		    auto existing = statsdata.find(asset_symbol_name);
		    eosio_assert(existing != statsdata.end(), "asset with symbol does not exist, create token before issue" );

		    // An asset can only be issued by the creator.
		    require_auth(existing->issuer);
		    eosio_assert(quantity.is_valid(), "invalid quantity" );
		    eosio_assert(quantity.amount > 0, "must issue positive quantity" );

		    eosio_assert(quantity.symbol == existing->supply.symbol, "symbol precision mismatch" );
		    eosio_assert(quantity.amount <= existing->max_supply.amount - existing->supply.amount, "quantity exceeds available supply");

		    statsdata.modify(*existing, existing->issuer, [&]( auto& s) {
		       s.supply += quantity;
		    });

		    add(existing->issuer, existing->issuer, quantity);

		    if(to != existing->issuer ) {
		    	action inline_action = action(
		    		permission_level{existing->issuer,"active"_n},
		    		// account that owns the contract. In this case, the name of the contract == name of the account
		    		_self,
				    "transferin"_n,
				    std::make_tuple(existing->issuer, to, quantity, memo)
				  );

		    	inline_action.send();
		    }
		}

		// move a specified quantity of an asset rom one account to another
		// @abi action
		[[eosio::action]]
		void transferin(const name& from, const name& to, const asset& quantity, const std::string& memo) {

			eosio_assert(from != to, "cannot transfer to self" );
			// sign transaction using 'from'
			require_auth(from);
			// receiver must exist
			eosio_assert(is_account(to), "to account does not exist");
			auto asset_symbol = quantity.symbol;
			stats statsdata(_self, _self.value);
			const auto& st = statsdata.get(asset_symbol.raw());

			// notify both the sender and receiver
			require_recipient(from);
			require_recipient(to);

			eosio_assert(quantity.is_valid(), "invalid quantity" );
			eosio_assert(quantity.amount > 0, "must transfer positive quantity" );
			eosio_assert(quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
			eosio_assert(memo.size() <= 256, "memo has more than 256 bytes" );

			// update sender balance
			sub(from, quantity);
			// update receiver balance
			add(from, to, quantity);
		}

		// create a deferred transaction.
		// @abi action
		[[eosio::action]]
		void transfer(const name& from, const name& to, const asset& quantity, const std::string& memo, uint64_t delay) {

			require_auth(from);
			transaction sched_tx{};
			sched_tx.actions.emplace_back(
				permission_level(_self, "active"_n), 
				_self,
				"transferin"_n,
				std::make_tuple(from, to, quantity, memo));

			sched_tx.delay_sec = delay;
			sched_tx.send(now(), from);
		}

		void add(const name& from, const name& to, const asset& value) {

			accounts accs(_self, to.value);
			auto _to = accs.find(value.symbol.raw());

			if(_to == accs.end()) {
				// this only happens when the account 'to' is receiving the asset for the
				// first time.
				// to == from
				accs.emplace(from, [&]( auto& a ){
					a.balance = value;
				});
			} else {
				accs.modify(_to, from, [&]( auto& a ) {
					a.balance += value;
				});
			}
		}

		void sub(const name& owner, const asset& value) {

			accounts accs(_self, owner.value);
			const auto& from = accs.get(value.symbol.raw(), "no balance object found" );
			eosio_assert(from.balance.amount >= value.amount, "overdrawn balance" );

			if(from.balance.amount == value.amount ) {
				accs.erase(from);
			} else {
				accs.modify(from, owner, [&](auto& a) {
					a.balance -= value;
				});
			}
		}

	private:
		// EOSIO Tables

		// A data structure that defines the amount / quantity of an asset held by
		// a single account.
		struct [[eosio::table]] account {
			asset    balance;

			uint64_t primary_key() const { return balance.symbol.raw(); }
		};

		// Each asset has some configuration; current amount of supply,
		// total amount that can be supplied and the name of the account 
		// that controls the supply of the asset.
		struct [[eosio::table]] asset_status {
			asset    supply;
			asset    max_supply;
			name     issuer;

			uint64_t primary_key() const { return supply.symbol.raw(); }
		};

		// Table configuration
		typedef eosio::multi_index<"accounts"_n, account> accounts;
		typedef eosio::multi_index<"stat"_n, asset_status> stats;
};

EOSIO_DISPATCH(assetex, (create)(issue)(transferin)(transfer))