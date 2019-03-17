#include <string>
#include <eosiolib/eosio.hpp>


using namespace eosio;

class [[eosio::contract("addressbook")]] addressbook : public contract {

	public:
		using contract::contract;

		addressbook(name recvr, name code, datastream<const char*> ds):contract(recvr, code, ds) {}

		// @abi action
		[[eosio::action]]
		void upsert(
			const name& user, const std::string& first_name, const std::string& last_name, const std::string& street,
			const std::string& city, const std::string& state) {

			// only the user has control over their own records
			require_auth(user);
			print("Name ==> ", user.value);
			// code --> name of account owning this contract
			// value accessed using the scoped '_code.value' variable
			address_index bk_addresses(_code, _code.value);

			// in Relational databases, this could be thought of as a 'SELECT * FROM PERSON WHERE PERSON.KEY == ?' clause,
			// however, the relation isn't that direct! 
			auto iter = bk_addresses.find(user.value);
			if (iter == bk_addresses.end()) {
				// create --> INSERT

				// pass all variables in scope 'by reference' to the callback / lambda expression
				bk_addresses.emplace(user, [&] (auto& newRow) {
					newRow.key = user;
					newRow.first_name = first_name;
					newRow.last_name = last_name;
					newRow.street = street;
					newRow.city = city;
					newRow.state = state;
				});
				print("New USER created.");
			} else {
				// update --> UPDATE
				bk_addresses.modify(iter, user, [&] (auto& row) {
					row.key = user;
					row.first_name = first_name;
					row.last_name = last_name;
					row.street = street;
					row.city = city;
					row.state = state;
				});
				print("Updates successful");
			}
		}

		// @abi action
		[[eosio::action]]
		void erase(const name& user) {
			require_auth(user);

			address_index bk_addresses(_code, _code.value);
			auto iter = bk_addresses.find(user.value);
			// a record can't be deleted if it doesn't exist
			eosio_assert(iter != bk_addresses.end(), "User doesn't exist");
			bk_addresses.erase(iter);
		}

	private:

		// data structure -- EOSIO TABLE
		struct [[eosio::table]] person {
			name key;

			std::string first_name;
			std::string last_name;
			std::string street;
			std::string city;
			std::string state;

			uint64_t primary_key() const { return key.value; }
		};

		// configure
		typedef eosio::multi_index<"people"_n, person> address_index;
};


EOSIO_DISPATCH(addressbook, (upsert)(erase));