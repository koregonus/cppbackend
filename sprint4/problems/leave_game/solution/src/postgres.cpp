#include "postgres.h"
#include <pqxx/zview.hxx>
#include <iostream>
#include "tagged_uuid.h"
namespace postgres
{

using namespace std::literals;
using pqxx::operator"" _zv;

void CreateTable(pqxx::connection& connection)
{
	pqxx::work work{connection};
	try
	{
		work.exec(R"(
CREATE TABLE IF NOT EXISTS retired_players (
id UUID CONSTRAINT firstindex PRIMARY KEY,
name varchar(100) NOT NULL,
score int,
playTime int
);
)"_zv);
	}
	catch(...)
	{
		std::cout << "err\n";
	}
work.exec(R"(
CREATE INDEX IF NOT EXISTS scores_rating ON
retired_players (score DESC, playtime, name)
;
)"_zv);
	// коммитим изменения
    work.commit();

}


void DumpLeftDogsToDatabase(pqxx::connection& conn, std::vector<model::DogLeftDump>& vec_input)
{
	pqxx::work work{conn};

	for(auto iter = vec_input.begin(); iter != vec_input.end(); iter++)
	{
		work.exec_params(
        R"(
INSERT INTO retired_players (id, name, score, playtime) VALUES ($1, $2, $3, $4)
)"_zv,
        util::detail::UUIDToString(util::detail::NewUUID()), iter->name, iter->score, (iter->play_time_ms > 0 ? iter->play_time_ms : 0));
	}
	work.commit();
}


void ReadScoresFromDatabase(pqxx::connection& conn, int offset, int limit, std::vector<model::DogLeftDump>& vec_input)
{
	pqxx::read_transaction read_trans(conn);
	auto query_text = "SELECT name, score, playtime FROM retired_players ORDER BY score DESC, playtime, name OFFSET " + read_trans.quote(offset) + " LIMIT " + read_trans.quote(limit) +
						";";
	for (auto [name, score, playtime] : read_trans.query<std::string, int, int>(query_text)) {

        vec_input.push_back({0, name, score, playtime});
    }
}








}

