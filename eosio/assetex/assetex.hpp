#ifndef _ASSETEX_H
#define _ASSETEX_H

#include <string>

#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/transaction.hpp>


using eosio::name;
using eosio::contract;
using eosio::asset;
using eosio::multi_index;
using eosio::datastream;
using eosio::permission_level;
using eosio::transaction;
using eosio::action;

namespace assetex {

	class [[eosio::contract("assetex")]] assetex : public contract {

		public:
			assetex(name self, name code, datastream<const char*> ds):contract(self, code, ds) {}

			[[eosio::action]] void create(const name& issuer, const asset& max_supply);
			[[eosio::action]] void issue(const name& to, const asset& quantity, const std::string& memo);
			[[eosio::action]] void transferin(const name& from, const name& to, const asset& quantity, const std::string& memo);
			[[eosio::action]] void transfer(const name& from, const name& to, const asset& quantity, const std::string& memo, uint64_t delay);
			void add(const name& from, const name& to, const asset& value);
			void sub(const name& owner, const asset& value);

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
			typedef multi_index<"accounts"_n, account> accounts;
			typedef multi_index<"stat"_n, asset_status> stats;
	};


	EOSIO_DISPATCH(assetex, (create)(issue)(transferin)(transfer))
}

#endif
