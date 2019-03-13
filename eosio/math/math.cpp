#include <eosiolib/eosio.hpp>

using namespace eosio;

class [[eosio::contract("math")]] math : public contract {
  public:
      using contract::contract;

      //@abi action
      [[eosio::action]]
      void add(int64_t a, int64_t b) {
         print(a, " + ", b, " = ", (a + b));
      }

      //@abi action
      [[eosio::action]]
      void sub(int64_t a, int64_t b) {
      	print(a, " - ", b, " = ", (a - b));
      }
};

EOSIO_DISPATCH(math, (add)(sub))