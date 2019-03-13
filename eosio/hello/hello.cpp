#include <eosiolib/eosio.hpp>

using namespace eosio;

class [[eosio::contract("hello")]] hello : public contract {
  public:
      using contract::contract;

      //@abi action
      [[eosio::action]]
      void hi( name user ) {
         print( "I'm just getting started, sweet, ", user, ". Welcome me please.");
      }
};

EOSIO_DISPATCH( hello, (hi))