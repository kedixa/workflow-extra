#include <string>
#include <signal.h>
#include <arpa/inet.h>

#include "workflow/WFFacilities.h"
#include "workflow/DnsMessage.h"
#include "workflow/DnsUtil.h"

using namespace std;
using protocol::DnsResultCursor;

string nameserver("8.8.8.8");
WFFacilities::WaitGroup wg(1);

void show_result(DnsResultCursor &cursor) {
    char information[1024];
    const char *info;
    struct dns_record *record;
    struct dns_record_soa *soa;
    struct dns_record_srv *srv;
    struct dns_record_mx *mx;

    while(cursor.next(&record))
    {
        switch (record->type) {
        case DNS_TYPE_A:
            info = inet_ntop(AF_INET, record->rdata, information, 64);
            break;
        case DNS_TYPE_AAAA:
            info = inet_ntop(AF_INET6, record->rdata, information, 64);
            break;
        case DNS_TYPE_NS:
        case DNS_TYPE_CNAME:
        case DNS_TYPE_PTR:
            info = (const char *)(record->rdata);
            break;
        case DNS_TYPE_SOA:
            soa = (struct dns_record_soa *)(record->rdata);
            sprintf(information, "%s %s %u %d %d %d %u",
                soa->mname, soa->rname, soa->serial, soa->refresh,
                soa->retry, soa->expire, soa->minimum
            );
            info = information;
            break;
        case DNS_TYPE_SRV:
            srv = (struct dns_record_srv *)(record->rdata);
            sprintf(information, "%u %u %u %s",
                srv->priority, srv->weight, srv->port, srv->target
            );
            info = information;
            break;
        case DNS_TYPE_MX:
            mx = (struct dns_record_mx *)(record->rdata);
            sprintf(information, "%d %s", mx->preference, mx->exchange);
            info = information;
            break;
        default:
            info = "Unknown";
        }
        printf("%s\t%d\t%s\t%s\t%s\n",
            record->name, record->ttl,
            dns_class2str(record->rclass),
            dns_type2str(record->type),
            info
        );
    }
    printf("\n");
}

void dns_callback(WFDnsTask *t) {
    int state = t->get_state();
    int error = t->get_error();
    auto *resp = t->get_resp();
    if (state != WFT_STATE_SUCCESS) {
        printf("State: %d, Error: %d\n", state, error);
        printf("Error: %s\n", WFGlobal::get_error_string(state, error));
        return;
    }

    printf(";  Workflow DNSResolver\n");
    printf(";; HEADER opcode:%s status:%s id:%d\n",
        dns_opcode2str(resp->get_opcode()),
        dns_rcode2str(resp->get_rcode()),
        resp->get_id()
    );
    printf(";; QUERY:%d ANSWER:%d AUTHORITY:%d ADDITIONAL:%d\n",
        resp->get_qdcount(), resp->get_ancount(),
        resp->get_nscount(), resp->get_arcount()
    );

    printf("\n");

    DnsResultCursor cursor(resp);
    if(resp->get_ancount() > 0) {
        cursor.reset_answer_cursor();
        printf(";; ANSWER SECTION:\n");
        show_result(cursor);
    }
    if(resp->get_nscount() > 0) {
        cursor.reset_authority_cursor();
        printf(";; AUTHORITY SECTION\n");
        show_result(cursor);
    }
    if(resp->get_arcount() > 0) {
        cursor.reset_additional_cursor();
        printf(";; ADDITIONAL SECTION\n");
        show_result(cursor);
    }
}

void sig_handler(int) {
    wg.done();
}

// Usage Example: ./wfdig zhihu.com A 8.8.8.8

int main(int argc, char *argv[]) {
    signal(SIGINT, sig_handler);
    WFGlobal::register_scheme_port("dns", 53);
    WFGlobal::register_scheme_port("dnss", 853);

    string host = "zhihu.com";
    string str_qtype = "A";
    string scheme = "dns";
    int qtype = DNS_TYPE_A;

    if(argc > 4) scheme = argv[4];
    if(argc > 3) nameserver = argv[3];
    if(argc > 2) str_qtype = argv[2];
    if(argc > 1) host = argv[1];

    if(str_qtype == "AAAA") qtype = DNS_TYPE_AAAA;
    else if(str_qtype == "SOA") qtype = DNS_TYPE_SOA;
    else if(str_qtype == "NS") qtype = DNS_TYPE_NS;
    else if(str_qtype == "TXT") qtype = DNS_TYPE_TXT;
    else if(str_qtype == "A") qtype = DNS_TYPE_A;
    else if(str_qtype == "CNAME") qtype = DNS_TYPE_CNAME;
    else if(str_qtype == "SRV") qtype = DNS_TYPE_SRV;
    else if(str_qtype == "ALL") qtype = DNS_TYPE_ALL;
    else if(str_qtype == "MX") qtype = DNS_TYPE_MX;

    if(scheme == "dnss") scheme = "dnss://";
    else scheme = "dns://";

    // dns[s]://nameserver[:port]/host.to.query
    string url = scheme + nameserver + "/" + host;

    auto *t = WFTaskFactory::create_dns_task(url, 0, dns_callback);
    t->set_receive_timeout(5000);
    auto *req = t->get_req();
    req->set_rd(1);
    req->set_question_type(qtype);

    auto series_callback = [](const SeriesWork *) {
        wg.done();
    };

    auto series = Workflow::create_series_work(t, series_callback);
    series->start();
    wg.wait();

    return 0;
}
