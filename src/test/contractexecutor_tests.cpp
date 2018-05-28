#include "contract/contractutil.h"
#include "contract/contractexecutor.h"
#include "test/test_bitcoin.h"
#include "util.h"
#include "utilstrencodings.h"
#include "validation.h"
#include <boost/test/unit_test.hpp>

dev::u256 GASLIMIT = dev::u256(500000);
dev::Address SENDERADDRESS = dev::Address("0101010101010101010101010101010101010101");
dev::h256 HASHTX = dev::h256(ParseHex("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));
std::vector<valtype> CODE =
{
    /*
        contract Temp {
            function () payable {}
        }
    */
    valtype(ParseHex("6060604052346000575b60398060166000396000f30060606040525b600b5b5b565b0000a165627a7a723058209cedb722bf57a30e3eb00eeefc392103ea791a2001deed29f5c3809ff10eb1dd0029")),
    /*
        contract Temp {
            function Temp(){
                while(true){

                }
            }
            function () payable {}
        }
    */
    valtype(ParseHex("6060604052346000575b5b600115601457600a565b5b5b60398060236000396000f30060606040525b600b5b5b565b0000a165627a7a7230582036c45484ccdd0a2e017c3d6842a4cf345f28abbb1eda4f27c4c26f2f8cf4dfe20029")),
    /*
        contract Temp {
            function () payable {
                while(true){

                }
            }
        }
    */
    valtype(ParseHex("6060604052346000575b60448060166000396000f30060606040525b60165b5b6001156013576009565b5b565b0000a165627a7a723058209aa6fe3625c7e2eac43ccddf503a8ab61af91a4fc757f5eac78b02412eb3c91b0029")),
    /*
        contract Factory {
            bytes32[] Names;
            address[] newContracts;

            function createContract (bytes32 name) {
                address newContract = new Contract(name);
                newContracts.push(newContract);
            }

            function getName (uint i) {
                Contract con = Contract(newContracts[i]);
                Names[i] = con.Name();
            }
        }

        contract Contract {
            bytes32 public Name;

            function Contract (bytes32 name) {
                Name = name;
            }

            function () payable {}
        }
    */
    valtype(ParseHex("606060405234610000575b61034a806100196000396000f30060606040526000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff1680633f811b80146100495780636b8ff5741461006a575b610000565b3461000057610068600480803560001916906020019091905050610087565b005b3461000057610085600480803590602001909190505061015b565b005b60008160405160e18061023e833901808260001916600019168152602001915050604051809103906000f08015610000579050600180548060010182818154818355818115116101035781836000526020600020918201910161010291905b808211156100fe5760008160009055506001016100e6565b5090565b5b505050916000526020600020900160005b83909190916101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff160217905550505b5050565b6000600182815481101561000057906000526020600020900160005b9054906101000a900473ffffffffffffffffffffffffffffffffffffffff1690508073ffffffffffffffffffffffffffffffffffffffff16638052474d6000604051602001526040518163ffffffff167c0100000000000000000000000000000000000000000000000000000000028152600401809050602060405180830381600087803b156100005760325a03f1156100005750505060405180519050600083815481101561000057906000526020600020900160005b5081600019169055505b50505600606060405234610000576040516020806100e1833981016040528080519060200190919050505b80600081600019169055505b505b609f806100426000396000f30060606040523615603d576000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff1680638052474d146045575b60435b5b565b005b34600057604f606d565b60405180826000191660001916815260200191505060405180910390f35b600054815600a165627a7a72305820fe28ec2b77f3b306095bda73561b85d147a1026db2e5714aeeb2f29246cffcbb0029a165627a7a7230582086cf938db13cf2aa8bca8ad6e720861683ef2cc971ad66dad68708438a5e4a9b0029")),
    /*
        contract sui {
            address addr = 0x382f0a81f70a2c43e652c353caf15494d1b57fae;

            function sui() payable {}

            function kill() payable {
                suicide(addr);
            }

            function () payable {}
        }
    */
    valtype(ParseHex("6060604052734de45add9f5f0b6887081cfcfe3aca6da9eb3365600060006101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff1602179055505b5b5b60b68061006a6000396000f30060606040523615603d576000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff16806341c0e1b5146045575b60435b5b565b005b604b604d565b005b600060009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16ff5b5600a165627a7a72305820e296f585c72ea3d4dce6880122cfe387d26c48b7960676a52e811b56ef8297a80029"))
};

static void checkExecResult(
        const std::vector<EthExecutionResult>& result,
        size_t execResSize,
        size_t addressesSize,
        dev::eth::TransactionException except,
        std::vector<dev::Address> newAddresses,
        valtype output,
        dev::u256 balance,
        bool normalAndIncorrect = false
        ) {
    std::unordered_map<dev::Address, dev::u256> addresses = EthState::Instance()->addresses();
    BOOST_CHECK(result.size() == execResSize);
    BOOST_CHECK(addresses.size() == addressesSize);
    for (size_t i = 0; i < result.size(); i++) {
        if ( normalAndIncorrect ) {
            if ( i%2 == 0 ) {
                BOOST_CHECK(result[i].execRes.excepted == dev::eth::TransactionException::None);
                BOOST_CHECK(result[i].execRes.newAddress == newAddresses[i]);
                BOOST_CHECK(result[i].execRes.output == output);
            } else {
                BOOST_CHECK(result[i].execRes.excepted == except);
                BOOST_CHECK(result[i].execRes.newAddress == dev::Address());
                BOOST_CHECK(result[i].execRes.output == valtype());
            }
        } else {
            BOOST_CHECK(result[i].execRes.excepted == except);
            BOOST_CHECK(result[i].execRes.newAddress == newAddresses[i]);
            BOOST_CHECK(result[i].execRes.output == output);
        }
        BOOST_CHECK(EthState::Instance()->balance(newAddresses[i]) == balance);
    }
}

static void checkBCEResult(const ExecutionResult& result, uint64_t usedGas, CAmount refundSender, size_t nVouts, uint64_t sum, size_t nTxs = 0) {
    BOOST_CHECK_MESSAGE(result.totalGasUsed + result.totalRefund == sum,
            "totalGasUsed:"<<result.totalGasUsed << " totalRefund:"<<result.totalRefund << " sum1:"<<sum << " sum2:"<<result.totalGasUsed + result.totalRefund);
    BOOST_CHECK_MESSAGE(result.totalGasUsed == usedGas, "totalGasUsed:"<<result.totalGasUsed << " usedGas:"<<usedGas);
    BOOST_CHECK_MESSAGE(result.totalRefund == refundSender, "totalRefund:"<<result.totalRefund << " refundSender:"<<refundSender);
    BOOST_CHECK_MESSAGE(result.refundTxOuts.size() == nVouts, "size:"<<result.refundTxOuts.size() << " nVouts:"<<nVouts);
    for (size_t i = 0; i < result.refundTxOuts.size(); i++) {
        BOOST_CHECK(result.refundTxOuts[i].scriptPubKey == CScript() << OP_DUP << OP_HASH160 << SENDERADDRESS.asBytes() << OP_EQUALVERIFY << OP_CHECKSIG);
    }
    BOOST_CHECK_MESSAGE(result.transferTxs.size() == nTxs, "size:"<<result.transferTxs.size() << " nTxs:"<<nTxs);
}

BOOST_FIXTURE_TEST_SUITE(contractexecutor_tests, TestingSetup)

BOOST_AUTO_TEST_CASE(contractexecutor_txs_empty) {
    std::vector<EthTransaction> txs;
    auto result = TestContractHelper::Execute(txs);
    BOOST_CHECK(result.first.size() == 0);
}

BOOST_AUTO_TEST_CASE(contractexecutor_create_contract) {
    std::vector<EthTransaction> txs = { TestContractHelper::CreateEthTx(CODE[0], 0, GASLIMIT, dev::u256(1), HASHTX, dev::Address()) };
    auto result = TestContractHelper::Execute(txs);

    std::vector<dev::Address> addrs = { ContractUtil::CreateContractAddr(txs[0].GetHashWith(), txs[0].GetOutIdx()) };
    valtype code = ParseHex("60606040525b600b5b5b565b0000a165627a7a723058209cedb722bf57a30e3eb00eeefc392103ea791a2001deed29f5c3809ff10eb1dd0029");
    checkExecResult(result.first, 1, 1, dev::eth::TransactionException::None, addrs, code, dev::u256(0));
    checkBCEResult(result.second, 69382, 43, 1, CAmount(69425));
}

BOOST_AUTO_TEST_CASE(contractexecutor_create_contract_OutOfGasBase){
    std::vector<EthTransaction> txs = { TestContractHelper::CreateEthTx(CODE[0], 0, dev::u256(100), dev::u256(1), HASHTX, dev::Address()) };
    auto result = TestContractHelper::Execute(txs);

    std::vector<dev::Address> addrs = { dev::Address() };
    checkExecResult(result.first, 1, 0, dev::eth::TransactionException::OutOfGasBase, addrs, valtype(), dev::u256(0));
    checkBCEResult(result.second, 100, 0, 0, 100);
}

BOOST_AUTO_TEST_CASE(contractexecutor_create_contract_OutOfGas) {
    std::vector<EthTransaction> txs = { TestContractHelper::CreateEthTx(CODE[1], 0, GASLIMIT, dev::u256(1), HASHTX, dev::Address()) };
    auto result = TestContractHelper::Execute(txs);

    std::vector<dev::Address> addrs = { dev::Address() };
    checkExecResult(result.first, 1, 0, dev::eth::TransactionException::OutOfGas, addrs, valtype(), dev::u256(0));
    checkBCEResult(result.second, CAmount(GASLIMIT), 0, 0, CAmount(GASLIMIT));
}

BOOST_AUTO_TEST_CASE(contractexecutor_OutOfGasBase_create_contract_normal_create_contract) {
    std::vector<dev::Address> newAddressGen;
    std::vector<EthTransaction> txs;
    dev::h256 hash(HASHTX);
    for (size_t i = 0; i < 10; i++) {
        dev::u256 gasLimit = i%2 == 0 ? GASLIMIT : dev::u256(100);
        EthTransaction txEth = TestContractHelper::CreateEthTx(CODE[0], 0, gasLimit, dev::u256(1), hash, dev::Address());
        newAddressGen.push_back(ContractUtil::CreateContractAddr(txEth.GetHashWith(), txEth.GetOutIdx()));
        txs.push_back(txEth);
        ++hash;
    }
    auto result = TestContractHelper::Execute(txs);

    valtype code = ParseHex("60606040525b600b5b5b565b0000a165627a7a723058209cedb722bf57a30e3eb00eeefc392103ea791a2001deed29f5c3809ff10eb1dd0029");
    checkExecResult(result.first, 10, 5, dev::eth::TransactionException::OutOfGasBase, newAddressGen, code, dev::u256(0), true);
    checkBCEResult(result.second, 347410, 215, 5, CAmount(347625));
}

BOOST_AUTO_TEST_CASE(contractexecutor_OutOfGas_create_contract_normal_create_contract) {
    std::vector<dev::Address> newAddressGen;
    std::vector<EthTransaction> txs;
    dev::h256 hash(HASHTX);
    for (size_t i = 0; i < 10; i++) {
        valtype code = i%2 == 0 ? CODE[0] : CODE[1];
        EthTransaction txEth = TestContractHelper::CreateEthTx(code, 0, GASLIMIT, dev::u256(1), hash, dev::Address());
        newAddressGen.push_back(ContractUtil::CreateContractAddr(txEth.GetHashWith(), txEth.GetOutIdx()));
        txs.push_back(txEth);
        ++hash;
    }
    auto result = TestContractHelper::Execute(txs);

    valtype code = ParseHex("60606040525b600b5b5b565b0000a165627a7a723058209cedb722bf57a30e3eb00eeefc392103ea791a2001deed29f5c3809ff10eb1dd0029");
    checkExecResult(result.first, 10, 5, dev::eth::TransactionException::OutOfGas, newAddressGen, code, dev::u256(0), true);
    checkBCEResult(result.second, 2846910, 215, 5, CAmount(2847125));
}

BOOST_AUTO_TEST_CASE(contractexecutor_create_contract_many){
    std::vector<dev::Address> newAddressGen;
    std::vector<EthTransaction> txs;
    dev::h256 hash(HASHTX);
    for (size_t i = 0; i < 130; i++) {
        EthTransaction txEth = TestContractHelper::CreateEthTx(CODE[0], 0, GASLIMIT, dev::u256(1), hash, dev::Address(), i);
        newAddressGen.push_back(ContractUtil::CreateContractAddr(txEth.GetHashWith(), txEth.GetOutIdx()));
        txs.push_back(txEth);
        ++hash;
    }
    auto result = TestContractHelper::Execute(txs);

    valtype code = ParseHex("60606040525b600b5b5b565b0000a165627a7a723058209cedb722bf57a30e3eb00eeefc392103ea791a2001deed29f5c3809ff10eb1dd0029");
    checkExecResult(result.first, 130, 130, dev::eth::TransactionException::None, newAddressGen, code, dev::u256(0));
    checkBCEResult(result.second, 9019660, 5590, 130, CAmount(9025250));
}

BOOST_AUTO_TEST_CASE(contractexecutor_call_contract_transfer){
    std::vector<EthTransaction> txsCreate = { TestContractHelper::CreateEthTx(CODE[0], 0, GASLIMIT, dev::u256(1), HASHTX, dev::Address()) };
    TestContractHelper::Execute(txsCreate);
    std::vector<dev::Address> addrs = { ContractUtil::CreateContractAddr(txsCreate[0].GetHashWith(), txsCreate[0].GetOutIdx()) };
    std::vector<EthTransaction> txsCall = { TestContractHelper::CreateEthTx(ParseHex("00"), 1300, GASLIMIT, dev::u256(1), HASHTX, addrs[0]) };
    auto result = TestContractHelper::Execute(txsCall);

    checkExecResult(result.first, 1, 1, dev::eth::TransactionException::None, addrs, valtype(), dev::u256(1300));
    checkBCEResult(result.second, 21037, 47, 1, CAmount(21084), 1);
}

BOOST_AUTO_TEST_CASE(contractexecutor_call_contract_transfer_OutOfGasBase_return_value){
    std::vector<EthTransaction> txsCreate = { TestContractHelper::CreateEthTx(CODE[0], 0, GASLIMIT, dev::u256(1), HASHTX, dev::Address()) };
    TestContractHelper::Execute(txsCreate);
    dev::Address newAddress(ContractUtil::CreateContractAddr(txsCreate[0].GetHashWith(), txsCreate[0].GetOutIdx()));
    std::vector<EthTransaction> txsCall = { TestContractHelper::CreateEthTx(ParseHex("00"), 1300, dev::u256(1), dev::u256(1), HASHTX, newAddress) };
    std::vector<dev::Address> addrs = { txsCall[0].receiveAddress() };
    auto result = TestContractHelper::Execute(txsCall);

    checkExecResult(result.first, 1, 1, dev::eth::TransactionException::OutOfGasBase, addrs, valtype(), dev::u256(0));
    checkBCEResult(result.second, 1, 0, 0, 1, 1);
}

BOOST_AUTO_TEST_CASE(contractexecutor_call_contract_transfer_OutOfGas_return_value){
    std::vector<EthTransaction> txsCreate = { TestContractHelper::CreateEthTx(CODE[2], 0, GASLIMIT, dev::u256(1), HASHTX, dev::Address()) };
    TestContractHelper::Execute(txsCreate);
    dev::Address newAddress(ContractUtil::CreateContractAddr(txsCreate[0].GetHashWith(), txsCreate[0].GetOutIdx()));
    std::vector<EthTransaction> txsCall = { TestContractHelper::CreateEthTx(ParseHex("00"), 1300, GASLIMIT, dev::u256(1), HASHTX, newAddress) };
    std::vector<dev::Address> addrs = { txsCall[0].receiveAddress() };
    auto result = TestContractHelper::Execute(txsCall);

    checkExecResult(result.first, 1, 1, dev::eth::TransactionException::OutOfGas, addrs, valtype(), dev::u256(0));
    checkBCEResult(result.second, CAmount(GASLIMIT), 0, 0, CAmount(GASLIMIT), 1);
}

BOOST_AUTO_TEST_CASE(contractexecutor_call_contract_transfer_many){
    std::vector<dev::Address> newAddressGen;
    std::vector<EthTransaction> txs;
    dev::h256 hash(HASHTX);
    for (size_t i = 0; i < 130; i++) {
        EthTransaction txEth = TestContractHelper::CreateEthTx(CODE[0], 0, GASLIMIT, dev::u256(1), hash, dev::Address(), i);
        newAddressGen.push_back(ContractUtil::CreateContractAddr(txEth.GetHashWith(), txEth.GetOutIdx()));
        txs.push_back(txEth);
        ++hash;
    }
    TestContractHelper::Execute(txs);
    std::vector<EthTransaction> txsCall;
    std::vector<dev::Address> addrs;
    valtype codeCall(ParseHex("00"));
    dev::h256 hashCall(HASHTX);
    for (size_t i = 0; i < txs.size(); i++) {
        EthTransaction txEthCall = TestContractHelper::CreateEthTx(codeCall, 1300, GASLIMIT, dev::u256(1), hash, newAddressGen[i], i);
        txsCall.push_back(txEthCall);
        addrs.push_back(txEthCall.receiveAddress());
        ++hashCall;
    }
    auto result = TestContractHelper::Execute(txsCall);

    checkExecResult(result.first, 130, 130, dev::eth::TransactionException::None, addrs, valtype(), dev::u256(1300));
    checkBCEResult(result.second, 2734810, 6110, 130, CAmount(2740920), 130);
}

BOOST_AUTO_TEST_CASE(contractexecutor_call_contract_OutOfGas_transfer_many_return_value){
    std::vector<dev::Address> newAddressGen;
    std::vector<EthTransaction> txs;
    dev::h256 hash(HASHTX);
    for (size_t i = 0; i < 130; i++) {
        EthTransaction txEth = TestContractHelper::CreateEthTx(CODE[2], 0, GASLIMIT, dev::u256(1), hash, dev::Address(), i);
        newAddressGen.push_back(ContractUtil::CreateContractAddr(txEth.GetHashWith(), txEth.GetOutIdx()));
        txs.push_back(txEth);
        ++hash;
    }
    TestContractHelper::Execute(txs);
    std::vector<EthTransaction> txsCall;
    std::vector<dev::Address> addrs;
    valtype codeCall(ParseHex("00"));
    dev::h256 hashCall(HASHTX);
    for (size_t i = 0; i < txs.size(); i++) {
        EthTransaction txEthCall = TestContractHelper::CreateEthTx(codeCall, 1300, GASLIMIT, dev::u256(1), hashCall, newAddressGen[i], i);
        txsCall.push_back(txEthCall);
        addrs.push_back(txEthCall.receiveAddress());
        ++hashCall;
    }
    auto result = TestContractHelper::Execute(txsCall);

    checkExecResult(result.first, 130, 130, dev::eth::TransactionException::OutOfGas, addrs, valtype(), dev::u256(0));
    checkBCEResult(result.second, CAmount(GASLIMIT) * 130, 0, 0, CAmount(GASLIMIT) * 130, 130);
}

BOOST_AUTO_TEST_CASE(contractexecutor_suicide){
    std::vector<EthTransaction> txsCreate;
    std::vector<dev::Address> newAddresses;
    dev::h256 hash(HASHTX);
    for (size_t i = 0; i < 10; i++) {
        valtype code = i == 0 ? CODE[0] : CODE[4];
        dev::u256 value = i == 0 ? dev::u256(0) : dev::u256(13);
        EthTransaction txEthCreate = TestContractHelper::CreateEthTx(code, 0, GASLIMIT, dev::u256(1), hash, dev::Address(), i);
        txsCreate.push_back(txEthCreate);

        EthTransaction txEthSendValue;
        if (i != 0) {
            txEthSendValue = TestContractHelper::CreateEthTx(valtype(), value, GASLIMIT, dev::u256(1), hash,
                    ContractUtil::CreateContractAddr(txEthCreate.GetHashWith(), txEthCreate.GetOutIdx()), i);
            txsCreate.push_back(txEthSendValue);
        }

        newAddresses.push_back(ContractUtil::CreateContractAddr(txEthCreate.GetHashWith(), txEthCreate.GetOutIdx()));
        ++hash;
    }

    TestContractHelper::Execute(txsCreate);

    std::vector<EthTransaction> txsCall;
    std::vector<dev::Address> addrs;
    valtype codeCall(ParseHex("41c0e1b5"));
    for (size_t i = 0; i < newAddresses.size() - 1; i++) {
        EthTransaction txEthCall = TestContractHelper::CreateEthTx(codeCall, 0, GASLIMIT, dev::u256(1), HASHTX, newAddresses[i + 1]);
        txsCall.push_back(txEthCall);
        addrs.push_back(txEthCall.receiveAddress());
    }
    auto result = TestContractHelper::Execute(txsCall);

    checkExecResult(result.first, 9, 1, dev::eth::TransactionException::None, addrs, valtype(), dev::u256(0));
    checkBCEResult(result.second, 248526, 423, 9, CAmount(248949), 9);
}

BOOST_AUTO_TEST_CASE(contractexecutor_contract_create_contracts){
    EthTransaction txEthCreate = TestContractHelper::CreateEthTx(CODE[3], 0, GASLIMIT, dev::u256(1), HASHTX, dev::Address());
    std::vector<EthTransaction> txs = { txEthCreate };
    TestContractHelper::Execute(txs);
    std::vector<EthTransaction> txsCall;
    valtype codeCall(ParseHex("3f811b80"));
    dev::Address newAddress(ContractUtil::CreateContractAddr(txEthCreate.GetHashWith(), txEthCreate.GetOutIdx()));
    std::vector<dev::Address> addrs;
    for (size_t i = 0; i < 20; i++) {
        EthTransaction txEthCall = TestContractHelper::CreateEthTx(codeCall, 0, GASLIMIT, dev::u256(1), HASHTX, newAddress, i);
        txsCall.push_back(txEthCall);
        addrs.push_back(txEthCall.receiveAddress());
    }
    auto result = TestContractHelper::Execute(txsCall);

    checkExecResult(result.first, 20, 21, dev::eth::TransactionException::None, addrs, valtype(), dev::u256(0));
    checkBCEResult(result.second, 2344520, 758, 20, CAmount(2345278));
}

BOOST_AUTO_TEST_SUITE_END()