#include "./long_poll.hpp"

using namespace cURL;
using std::to_string;
using nlohmann::json;

void Lp::get_lp_server() {
  params body{{ "group_id", group_id }};
  append_vkparams(body);
  string url = append_vkurl("groups.getLongPollServer");
  json poll;
  try {
    poll  = json::parse(request(url, body));
  }
  catch(json::parse_error& parse_error) {
    logger.write_err(__LINE__, __FILE__, __FUNCTION__, parse_error.what());
  }
  if (not poll["error"]["error_code"].is_null()) {
    errors_handle(poll["error"]["error_code"]);
  }
  server = poll["response"]["server"];
  key    = poll["response"]["key"];
  ts     = to_string(poll["response"]["ts"].get<long>());
}

void Lp::loop() {
  get_lp_server();
  while (true) {
    params lp_body;
    lp_body["act"]  = "a_check";
    lp_body["key"]  = key;
    lp_body["ts"]   = ts;
    lp_body["wait"] = "60";

    json lp = json::parse(request(server + "?", lp_body));
    if (lp["updates"][0].is_null()) {
      get_lp_server();
    } else {
      ts = lp["ts"];
      for (auto update : lp["updates"]) {
        if (update["object"]["message"]["text"] != "") {
          handler.init_cmds(
            update["object"]["message"]["text"],
            update["object"]["message"]["peer_id"],
            update["object"]["message"]["from_id"]
          );
        }
      }
    }
  }
}

void Lp::init_bot() {
  Database db;
  db.open();
  db.init_table();
  loop();
}
