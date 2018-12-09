#include <iostream>
#include <fstream>
#include <string>
#include <cassert>
#include <vector>
#include <algorithm>
#include <cmath>
#include <stdlib.h>
#include <time.h>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/gnuplot.h"
#include "ns3/random-variable-stream.h"
#include "ns3/random-variable-stream-helper.h"
#include "ns3/ptr.h"
#include "ns3/double.h"
#include "ns3/mobility-module.h" 
#include "ns3/netanim-module.h"
#include "ns3/delay-jitter-estimation.h"
#include "ns3/rip.h"
#include "ns3/trace-helper.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-routing-table-entry.h"
#include "ns3/csma-module.h"
#include "ns3/net-device.h"
#include "ns3/object.h"
#include "ns3/packet.h"
#include "ns3/timer.h"
#include "ns3/nstime.h"
#include "ns3/flow-monitor-helper.h"

using namespace ns3;
using namespace std;
NS_LOG_COMPONENT_DEFINE ("Homework");

class routerList
{
	public:
	int nHosts;
	int nRouters_0;
	int nRouters_1;
	int connectHosts[5];
	int connectRouters_0[5];
	int connectRouters_1[5];
};

vector<routerList> RouterLists(30);
NodeContainer routers;
NodeContainer hosts;
PointToPointHelper pointToPoint;
vector<NetDeviceContainer> devicesHosts(50);
vector<NetDeviceContainer> devicesRouters(50);
vector<NetDeviceContainer> wholeRouters(50);
uint32_t maxQ[100] = {0};
int pt = 0;
int uid_last=0;
int uid_terlast=0;
int uid_now=0;
int hostPairs=0;
double sendTime[20000];
double receiveTime[20000];
int minMax[30];
int countt=-1;
int counttt=-1;
void getminmaxQ()
{
	int len=0;
  for(uint32_t i=0;i<30;i++)
  {
	len=0;
	int temp;
	for(int k=0;k<RouterLists[i].nHosts;k++)
	{
			PointerValue ptr;
			devicesHosts[RouterLists[i].connectHosts[k]].Get(0)->GetAttribute("TxQueue",ptr);
    			Ptr<Queue> txQueue = ptr.Get<Queue>();
			temp=txQueue->GetNPackets();
			if(temp>len)
				len=temp;
	 }
	for(int k=0;k<RouterLists[i].nRouters_0;k++)
	{
			PointerValue ptr;
			devicesRouters[RouterLists[i].connectRouters_0[k]].Get(0)->GetAttribute("TxQueue",ptr);
    			Ptr<Queue> txQueue = ptr.Get<Queue>();
			temp=txQueue->GetNPackets();
			if(temp>len)
				len=temp;
	}
	for(int k=0;k<RouterLists[i].nRouters_1;k++)
	{
			PointerValue ptr;
			devicesRouters[RouterLists[i].connectRouters_1[k]].Get(1)->GetAttribute("TxQueue",ptr);
    			Ptr<Queue> txQueue = ptr.Get<Queue>();
			temp=txQueue->GetNPackets();
			if(temp>len)
				len=temp;
	}
	minMax[i]=len;
  }
  countt++;
	int max=0;
	if(countt==100)
	{for(int i=0;i<30;i++)
	{
		if(minMax[i]>max)
		max=minMax[i];
//cout<<minMax[i]<<endl;

		minMax[i]=0;
	}
	cout <<Simulator::Now().GetSeconds()<<"\t" << max << endl;

	countt=0;
	}
}


void calThroughput()
{
	double throughput_now=(uid_now-uid_terlast)/hostPairs;
	cout <<Simulator::Now().GetSeconds()<<"\t" <<throughput_now<<endl;
	uid_terlast=uid_now;
	if(counttt++==10)
	{
		uid_last=uid_now;
		counttt=0;
	}

}
void calDelay()
{
	int iterDelay;
	double delayMin_now=9;
	if(uid_now-uid_last>19999)
		iterDelay=19999;
	else	iterDelay=uid_now-uid_last;
	for(int i=1;i<iterDelay;i++)
	{
		double thisDelay=receiveTime[i]-sendTime[i];
		if(thisDelay > 0)
		{
			if(delayMin_now>thisDelay)
				delayMin_now=thisDelay;
		}

	}
	if(delayMin_now==9)
		delayMin_now=1;
	cout <<Simulator::Now().GetSeconds()<<"\t" <<delayMin_now<<endl;
	uid_last=uid_now;
}
class MyApp : public Application 
{
public:

  MyApp ();
  virtual ~MyApp();

  void Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate);

private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void ScheduleTx (void);
  void SendPacket (void);

  Ptr<Socket>     m_socket;
  Address         m_peer;
  uint32_t        m_packetSize;
  uint32_t        m_nPackets;
  DataRate        m_dataRate;
  EventId         m_sendEvent;
  bool            m_running;
  uint32_t        m_packetsSent;
};

MyApp::MyApp ()
  : m_socket (0), 
    m_peer (), 
    m_packetSize (0), 
    m_nPackets (0), 
    m_dataRate (0), 
    m_sendEvent (), 
    m_running (false), 
    m_packetsSent (0)
{
}

MyApp::~MyApp()
{
  m_socket = 0;
}

void
MyApp::Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate)
{
  m_socket = socket;
  m_peer = address;
  m_packetSize = packetSize;
  m_nPackets = nPackets;
  m_dataRate = dataRate;
}

void
MyApp::StartApplication (void)
{
  m_running = true;
  m_packetsSent = 0;
  m_socket->Bind ();
  m_socket->Connect (m_peer);
  SendPacket ();
}

void 
MyApp::StopApplication (void)
{
  m_running = false;

  if (m_sendEvent.IsRunning ())
    {
      Simulator::Cancel (m_sendEvent);
    }

  if (m_socket)
    {
      m_socket->Close ();
    }
}

void 
MyApp::SendPacket (void)
{
  Ptr<Packet> packet = Create<Packet> (m_packetSize);
  int uid = packet->GetUid();
  m_socket->Send (packet);
  if(uid>uid_last&&uid-uid_last<20000)
  	sendTime[uid-uid_last] = Simulator::Now().GetSeconds();
  if (++m_packetsSent < m_nPackets)
    {
      ScheduleTx ();
    }
}

void 
MyApp::ScheduleTx (void)
{
  if (m_running)
    {
      //Time tNext (Seconds (m_packetSize * 8 / static_cast<double> (m_dataRate.GetBitRate ())));
	  Time tNext (Seconds (0.00375));
      m_sendEvent = Simulator::Schedule (tNext, &MyApp::SendPacket, this);
    }
}

static void
configPacket (Ptr<const Packet> p,const Address &src)
{
	uid_now=p->GetUid();
	if(uid_now>uid_last&&uid_now-uid_last<20000)
  	receiveTime[uid_now-uid_last] = Simulator::Now().GetSeconds();
}

void SetUpp()
{
	RouterLists[0].nHosts=2;	
	RouterLists[1].nHosts=2;
	RouterLists[2].nHosts=2;
	RouterLists[3].nHosts=2;
	RouterLists[4].nHosts=2;
	RouterLists[5].nHosts=1;
	RouterLists[6].nHosts=0;
	RouterLists[7].nHosts=3;
	RouterLists[8].nHosts=3;
	RouterLists[9].nHosts=1;
	RouterLists[10].nHosts=2;
	RouterLists[11].nHosts=1;
	RouterLists[12].nHosts=1;
	RouterLists[13].nHosts=2;
	RouterLists[14].nHosts=1;
	RouterLists[15].nHosts=3;
	RouterLists[16].nHosts=3;
	RouterLists[17].nHosts=1;
	RouterLists[18].nHosts=1;
	RouterLists[19].nHosts=1;
	RouterLists[20].nHosts=4;
	RouterLists[21].nHosts=1;
	RouterLists[22].nHosts=0;
	RouterLists[23].nHosts=1;
	RouterLists[24].nHosts=1;
	RouterLists[25].nHosts=1;
	RouterLists[26].nHosts=1;
	RouterLists[27].nHosts=1;
	RouterLists[28].nHosts=3;
	RouterLists[29].nHosts=3;

	RouterLists[0].connectHosts[0]=0;
	RouterLists[0].connectHosts[1]=1;
	RouterLists[1].connectHosts[0]=2;
	RouterLists[1].connectHosts[1]=3;
	RouterLists[2].connectHosts[0]=5;
	RouterLists[2].connectHosts[1]=4;
	RouterLists[3].connectHosts[0]=7;
	RouterLists[3].connectHosts[1]=6;
	RouterLists[4].connectHosts[0]=8;
	RouterLists[4].connectHosts[1]=9;
	RouterLists[5].connectHosts[0]=10;
	RouterLists[7].connectHosts[0]=11;
	RouterLists[7].connectHosts[1]=12;
	RouterLists[7].connectHosts[2]=13;
	RouterLists[8].connectHosts[0]=14;
	RouterLists[8].connectHosts[1]=15;
	RouterLists[8].connectHosts[2]=16;
	RouterLists[9].connectHosts[0]=18;
	RouterLists[10].connectHosts[0]=19;
	RouterLists[10].connectHosts[1]=20;
	RouterLists[11].connectHosts[0]=21;
	RouterLists[12].connectHosts[0]=17;
	RouterLists[13].connectHosts[0]=22;
	RouterLists[13].connectHosts[1]=23;
	RouterLists[14].connectHosts[0]=24;
	RouterLists[15].connectHosts[0]=25;
	RouterLists[15].connectHosts[1]=26;
	RouterLists[15].connectHosts[2]=27;
	RouterLists[16].connectHosts[0]=28;
	RouterLists[16].connectHosts[1]=29;
	RouterLists[16].connectHosts[2]=30;
	RouterLists[17].connectHosts[0]=31;
	RouterLists[18].connectHosts[0]=32;
	RouterLists[19].connectHosts[0]=33;
	RouterLists[20].connectHosts[0]=34;
	RouterLists[20].connectHosts[1]=35;
	RouterLists[20].connectHosts[2]=36;
	RouterLists[20].connectHosts[3]=37;
	RouterLists[21].connectHosts[0]=38;
	RouterLists[23].connectHosts[0]=47;
	RouterLists[24].connectHosts[0]=49;
	RouterLists[25].connectHosts[0]=39;
	RouterLists[26].connectHosts[0]=48;
	RouterLists[27].connectHosts[0]=43;
	RouterLists[28].connectHosts[0]=44;
	RouterLists[28].connectHosts[1]=45;
	RouterLists[28].connectHosts[2]=46;
	RouterLists[29].connectHosts[0]=40;
	RouterLists[29].connectHosts[1]=41;
	RouterLists[29].connectHosts[2]=42;

	RouterLists[0].nRouters_0=3;	
	RouterLists[1].nRouters_0=1;
	RouterLists[2].nRouters_0=2;
	RouterLists[3].nRouters_0=1;
	RouterLists[4].nRouters_0=2;
	RouterLists[5].nRouters_0=1;
	RouterLists[6].nRouters_0=2;
	RouterLists[7].nRouters_0=1;
	RouterLists[8].nRouters_0=2;
	RouterLists[9].nRouters_0=2;
	RouterLists[10].nRouters_0=3;
	RouterLists[11].nRouters_0=2;
	RouterLists[12].nRouters_0=1;
	RouterLists[13].nRouters_0=2;
	RouterLists[14].nRouters_0=0;
	RouterLists[15].nRouters_0=1;
	RouterLists[16].nRouters_0=2;
	RouterLists[17].nRouters_0=2;
	RouterLists[18].nRouters_0=2;
	RouterLists[19].nRouters_0=3;
	RouterLists[20].nRouters_0=1;
	RouterLists[21].nRouters_0=3;
	RouterLists[22].nRouters_0=2;
	RouterLists[23].nRouters_0=2;
	RouterLists[24].nRouters_0=1;
	RouterLists[25].nRouters_0=2;
	RouterLists[26].nRouters_0=2;
	RouterLists[27].nRouters_0=1;
	RouterLists[28].nRouters_0=1;
	RouterLists[29].nRouters_0=0;

	RouterLists[0].connectRouters_0[0]=0;
	RouterLists[0].connectRouters_0[1]=1;
	RouterLists[0].connectRouters_0[2]=2;
	RouterLists[1].connectRouters_0[0]=3;
	RouterLists[2].connectRouters_0[0]=4;
	RouterLists[2].connectRouters_0[1]=5;
	RouterLists[3].connectRouters_0[0]=6;
	RouterLists[4].connectRouters_0[0]=7;
	RouterLists[4].connectRouters_0[1]=8;
	RouterLists[5].connectRouters_0[0]=9;
	RouterLists[6].connectRouters_0[0]=10;
	RouterLists[6].connectRouters_0[1]=11;
	RouterLists[7].connectRouters_0[0]=12;
	RouterLists[8].connectRouters_0[0]=13;
	RouterLists[8].connectRouters_0[1]=14;
	RouterLists[9].connectRouters_0[0]=15;
	RouterLists[9].connectRouters_0[1]=16;
	RouterLists[10].connectRouters_0[0]=17;
	RouterLists[10].connectRouters_0[1]=18;
	RouterLists[10].connectRouters_0[2]=19;
	RouterLists[11].connectRouters_0[0]=20;
	RouterLists[11].connectRouters_0[1]=21;
	RouterLists[12].connectRouters_0[0]=22;
	RouterLists[13].connectRouters_0[0]=23;
	RouterLists[13].connectRouters_0[1]=24;
	RouterLists[15].connectRouters_0[0]=25;
	RouterLists[16].connectRouters_0[0]=26;
	RouterLists[16].connectRouters_0[1]=27;
	RouterLists[17].connectRouters_0[0]=28;
	RouterLists[17].connectRouters_0[1]=29;
	RouterLists[18].connectRouters_0[0]=30;
	RouterLists[18].connectRouters_0[1]=31;
	RouterLists[19].connectRouters_0[0]=32;
	RouterLists[19].connectRouters_0[1]=33;
	RouterLists[19].connectRouters_0[2]=34;
	RouterLists[20].connectRouters_0[0]=35;
	RouterLists[21].connectRouters_0[0]=36;
	RouterLists[21].connectRouters_0[1]=37;
	RouterLists[21].connectRouters_0[2]=38;
	RouterLists[22].connectRouters_0[0]=39;
	RouterLists[22].connectRouters_0[1]=40;
	RouterLists[23].connectRouters_0[0]=41;
	RouterLists[23].connectRouters_0[1]=42;
	RouterLists[24].connectRouters_0[0]=43;
	RouterLists[25].connectRouters_0[0]=44;
	RouterLists[25].connectRouters_0[1]=45;
	RouterLists[26].connectRouters_0[0]=46;
	RouterLists[26].connectRouters_0[1]=47;
	RouterLists[27].connectRouters_0[0]=48;
	RouterLists[28].connectRouters_0[0]=49;

	RouterLists[0].nRouters_1=0;	
	RouterLists[1].nRouters_1=1;
	RouterLists[2].nRouters_1=1;
	RouterLists[3].nRouters_1=2;
	RouterLists[4].nRouters_1=1;
	RouterLists[5].nRouters_1=2;
	RouterLists[6].nRouters_1=1;
	RouterLists[7].nRouters_1=2;
	RouterLists[8].nRouters_1=1;
	RouterLists[9].nRouters_1=1;
	RouterLists[10].nRouters_1=1;
	RouterLists[11].nRouters_1=2;
	RouterLists[12].nRouters_1=3;
	RouterLists[13].nRouters_1=2;
	RouterLists[14].nRouters_1=1;
	RouterLists[15].nRouters_1=1;
	RouterLists[16].nRouters_1=1;
	RouterLists[17].nRouters_1=1;
	RouterLists[18].nRouters_1=2;
	RouterLists[19].nRouters_1=3;
	RouterLists[20].nRouters_1=1;
	RouterLists[21].nRouters_1=1;
	RouterLists[22].nRouters_1=3;
	RouterLists[23].nRouters_1=2;
	RouterLists[24].nRouters_1=3;
	RouterLists[25].nRouters_1=1;
	RouterLists[26].nRouters_1=1;
	RouterLists[27].nRouters_1=3;
	RouterLists[28].nRouters_1=3;
	RouterLists[29].nRouters_1=3;

	RouterLists[1].connectRouters_1[0]=0;
	RouterLists[2].connectRouters_1[0]=3;
	RouterLists[3].connectRouters_1[0]=1;
	RouterLists[3].connectRouters_1[1]=4;
	RouterLists[4].connectRouters_1[0]=2;
	RouterLists[5].connectRouters_1[0]=6;
	RouterLists[5].connectRouters_1[1]=7;
	RouterLists[6].connectRouters_1[0]=9;
	RouterLists[7].connectRouters_1[0]=5;
	RouterLists[7].connectRouters_1[1]=10;
	RouterLists[8].connectRouters_1[0]=8;
	RouterLists[9].connectRouters_1[0]=12;
	RouterLists[10].connectRouters_1[0]=13;
	RouterLists[11].connectRouters_1[0]=14;
	RouterLists[11].connectRouters_1[1]=18;
	RouterLists[12].connectRouters_1[0]=11;
	RouterLists[12].connectRouters_1[1]=15;
	RouterLists[12].connectRouters_1[2]=20;
	RouterLists[13].connectRouters_1[0]=19;
	RouterLists[13].connectRouters_1[1]=21;
	RouterLists[14].connectRouters_1[0]=23;
	RouterLists[15].connectRouters_1[0]=15;
	RouterLists[16].connectRouters_1[0]=16;
	RouterLists[17].connectRouters_1[0]=26;
	RouterLists[18].connectRouters_1[0]=27;
	RouterLists[18].connectRouters_1[0]=28;
	RouterLists[19].connectRouters_1[0]=22;
	RouterLists[19].connectRouters_1[1]=25;
	RouterLists[19].connectRouters_1[2]=30;
	RouterLists[20].connectRouters_1[0]=32;
	RouterLists[21].connectRouters_1[0]=17;
	RouterLists[22].connectRouters_1[0]=29;
	RouterLists[22].connectRouters_1[1]=31;
	RouterLists[22].connectRouters_1[2]=36;
	RouterLists[23].connectRouters_1[0]=34;
	RouterLists[23].connectRouters_1[1]=39;
	RouterLists[24].connectRouters_1[0]=37;
	RouterLists[24].connectRouters_1[1]=40;
	RouterLists[24].connectRouters_1[2]=41;
	RouterLists[25].connectRouters_1[0]=33;
	RouterLists[26].connectRouters_1[0]=43;
	RouterLists[27].connectRouters_1[0]=42;
	RouterLists[27].connectRouters_1[1]=44;
	RouterLists[27].connectRouters_1[2]=46;
	RouterLists[28].connectRouters_1[0]=35;
	RouterLists[28].connectRouters_1[1]=45;
	RouterLists[28].connectRouters_1[2]=48;
	RouterLists[29].connectRouters_1[0]=38;
	RouterLists[29].connectRouters_1[1]=47;
	RouterLists[29].connectRouters_1[2]=49;
}



int
main(int argc, char *argv[])
{
	std::srand((int)time(NULL));
	SetUpp();
  bool verbose = false;
  bool printRoutingTables = false;
  bool showPings = false;
  std::string SplitHorizon ("PoisonReverse");

  CommandLine cmd;
  cmd.AddValue ("verbose", "turn on log components", verbose);
  cmd.AddValue ("printRoutingTables", "Print routing tables at 30, 60 and 90 seconds", printRoutingTables);
  cmd.AddValue ("showPings", "Show Ping6 reception", showPings);
  cmd.AddValue ("splitHorizonStrategy", "Split Horizon strategy to use (NoSplitHorizon, SplitHorizon, PoisonReverse)", SplitHorizon);
  cmd.Parse (argc, argv);

  if (verbose)
    {
      LogComponentEnableAll (LogLevel (LOG_PREFIX_TIME | LOG_PREFIX_NODE));
      LogComponentEnable ("RipSimpleRouting", LOG_LEVEL_INFO);
      LogComponentEnable ("Rip", LOG_LEVEL_ALL);
      LogComponentEnable ("Ipv4Interface", LOG_LEVEL_ALL);
      LogComponentEnable ("Icmpv4L4Protocol", LOG_LEVEL_ALL);
      LogComponentEnable ("Ipv4L3Protocol", LOG_LEVEL_ALL);
      LogComponentEnable ("ArpCache", LOG_LEVEL_ALL);
      LogComponentEnable ("V4Ping", LOG_LEVEL_ALL);
    }

  if (SplitHorizon == "NoSplitHorizon")
    {
      Config::SetDefault ("ns3::Rip::SplitHorizon", EnumValue (RipNg::NO_SPLIT_HORIZON));
    }
  else if (SplitHorizon == "SplitHorizon")
    {
      Config::SetDefault ("ns3::Rip::SplitHorizon", EnumValue (RipNg::SPLIT_HORIZON));
    }
  else
    {
      Config::SetDefault ("ns3::Rip::SplitHorizon", EnumValue (RipNg::POISON_REVERSE));
    }
	//create hosts and routers nodes

	hosts.Create(50);
	routers.Create(30);

	//assign every hosts to its routers
	vector<NodeContainer> routerToHost(50);
	routerToHost[0] = NodeContainer(routers.Get(0), hosts.Get(0));
	routerToHost[1] = NodeContainer(routers.Get(0), hosts.Get(1));
	routerToHost[2] = NodeContainer(routers.Get(1), hosts.Get(2));
	routerToHost[3] = NodeContainer(routers.Get(1), hosts.Get(3));
	routerToHost[5] = NodeContainer(routers.Get(2), hosts.Get(5));
	routerToHost[4] = NodeContainer(routers.Get(2), hosts.Get(4));
	routerToHost[7] = NodeContainer(routers.Get(3), hosts.Get(7));
	routerToHost[6] = NodeContainer(routers.Get(3), hosts.Get(6));
	routerToHost[8] = NodeContainer(routers.Get(4), hosts.Get(8));
	routerToHost[9] = NodeContainer(routers.Get(4), hosts.Get(9));
	routerToHost[10] = NodeContainer(routers.Get(5), hosts.Get(10));
	routerToHost[11] = NodeContainer(routers.Get(7), hosts.Get(11));
	routerToHost[12] = NodeContainer(routers.Get(7), hosts.Get(12));
	routerToHost[13] = NodeContainer(routers.Get(7), hosts.Get(13));
	routerToHost[14] = NodeContainer(routers.Get(8), hosts.Get(14));
	routerToHost[15] = NodeContainer(routers.Get(8), hosts.Get(15));
	routerToHost[16] = NodeContainer(routers.Get(8), hosts.Get(16));
	routerToHost[18] = NodeContainer(routers.Get(9), hosts.Get(18));
	routerToHost[19] = NodeContainer(routers.Get(10), hosts.Get(19));
	routerToHost[20] = NodeContainer(routers.Get(10), hosts.Get(20));
	routerToHost[21] = NodeContainer(routers.Get(11), hosts.Get(21));
	routerToHost[17] = NodeContainer(routers.Get(12), hosts.Get(17));
	routerToHost[22] = NodeContainer(routers.Get(13), hosts.Get(22));
	routerToHost[23] = NodeContainer(routers.Get(13), hosts.Get(23));
	routerToHost[24] = NodeContainer(routers.Get(14), hosts.Get(24));
	routerToHost[25] = NodeContainer(routers.Get(15), hosts.Get(25));
	routerToHost[26] = NodeContainer(routers.Get(15), hosts.Get(26));
	routerToHost[27] = NodeContainer(routers.Get(15), hosts.Get(27));
	routerToHost[28] = NodeContainer(routers.Get(16), hosts.Get(28));
	routerToHost[29] = NodeContainer(routers.Get(16), hosts.Get(29));
	routerToHost[30] = NodeContainer(routers.Get(16), hosts.Get(30));
	routerToHost[31] = NodeContainer(routers.Get(17), hosts.Get(31));
	routerToHost[32] = NodeContainer(routers.Get(18), hosts.Get(32));
	routerToHost[33] = NodeContainer(routers.Get(19), hosts.Get(33));
	routerToHost[34] = NodeContainer(routers.Get(20), hosts.Get(34));
	routerToHost[35] = NodeContainer(routers.Get(20), hosts.Get(35));
	routerToHost[36] = NodeContainer(routers.Get(20), hosts.Get(36));
	routerToHost[37] = NodeContainer(routers.Get(20), hosts.Get(37));
	routerToHost[38] = NodeContainer(routers.Get(21), hosts.Get(38));
	routerToHost[47] = NodeContainer(routers.Get(23), hosts.Get(47));
	routerToHost[49] = NodeContainer(routers.Get(24), hosts.Get(49));
	routerToHost[39] = NodeContainer(routers.Get(25), hosts.Get(39));
	routerToHost[48] = NodeContainer(routers.Get(26), hosts.Get(48));
	routerToHost[43] = NodeContainer(routers.Get(27), hosts.Get(43));
	routerToHost[44] = NodeContainer(routers.Get(28), hosts.Get(44));
	routerToHost[45] = NodeContainer(routers.Get(28), hosts.Get(45));
	routerToHost[46] = NodeContainer(routers.Get(28), hosts.Get(46));
	routerToHost[40] = NodeContainer(routers.Get(29), hosts.Get(40));
	routerToHost[41] = NodeContainer(routers.Get(29), hosts.Get(41));
	routerToHost[42] = NodeContainer(routers.Get(29), hosts.Get(42));

	// assign router channels
	vector<NodeContainer> routerToRouter(50);
	routerToRouter[0] = NodeContainer(routers.Get(0), routers.Get(1));
	routerToRouter[1] = NodeContainer(routers.Get(0), routers.Get(3));
	routerToRouter[2] = NodeContainer(routers.Get(0), routers.Get(4));
	routerToRouter[3] = NodeContainer(routers.Get(1), routers.Get(2));
	routerToRouter[4] = NodeContainer(routers.Get(2), routers.Get(3));
	routerToRouter[5] = NodeContainer(routers.Get(2), routers.Get(7));
	routerToRouter[6] = NodeContainer(routers.Get(3), routers.Get(5));
	routerToRouter[7] = NodeContainer(routers.Get(4), routers.Get(5));
	routerToRouter[8] = NodeContainer(routers.Get(4), routers.Get(8));
	routerToRouter[9] = NodeContainer(routers.Get(5), routers.Get(6));
	routerToRouter[10] = NodeContainer(routers.Get(6), routers.Get(7));
	routerToRouter[11] = NodeContainer(routers.Get(6), routers.Get(12));
	routerToRouter[12] = NodeContainer(routers.Get(7), routers.Get(9));
	routerToRouter[13] = NodeContainer(routers.Get(8), routers.Get(10));
	routerToRouter[14] = NodeContainer(routers.Get(8), routers.Get(11));
	routerToRouter[15] = NodeContainer(routers.Get(9), routers.Get(12));
	routerToRouter[16] = NodeContainer(routers.Get(9), routers.Get(15));
	routerToRouter[17] = NodeContainer(routers.Get(10), routers.Get(21));
	routerToRouter[18] = NodeContainer(routers.Get(10), routers.Get(11));
	routerToRouter[19] = NodeContainer(routers.Get(10), routers.Get(13));
	routerToRouter[20] = NodeContainer(routers.Get(11), routers.Get(12));
	routerToRouter[21] = NodeContainer(routers.Get(11), routers.Get(13));
	routerToRouter[22] = NodeContainer(routers.Get(12), routers.Get(19));
	routerToRouter[23] = NodeContainer(routers.Get(13), routers.Get(14));
	routerToRouter[24] = NodeContainer(routers.Get(13), routers.Get(16));
	routerToRouter[25] = NodeContainer(routers.Get(15), routers.Get(19));
	routerToRouter[26] = NodeContainer(routers.Get(16), routers.Get(17));
	routerToRouter[27] = NodeContainer(routers.Get(16), routers.Get(18));
	routerToRouter[28] = NodeContainer(routers.Get(17), routers.Get(18));
	routerToRouter[29] = NodeContainer(routers.Get(17), routers.Get(22));
	routerToRouter[30] = NodeContainer(routers.Get(18), routers.Get(19));
	routerToRouter[31] = NodeContainer(routers.Get(18), routers.Get(22));
	routerToRouter[32] = NodeContainer(routers.Get(19), routers.Get(20));
	routerToRouter[33] = NodeContainer(routers.Get(19), routers.Get(25));
	routerToRouter[34] = NodeContainer(routers.Get(19), routers.Get(23));
	routerToRouter[35] = NodeContainer(routers.Get(20), routers.Get(28));
	routerToRouter[36] = NodeContainer(routers.Get(21), routers.Get(22));
	routerToRouter[37] = NodeContainer(routers.Get(21), routers.Get(24));
	routerToRouter[38] = NodeContainer(routers.Get(21), routers.Get(29));
	routerToRouter[39] = NodeContainer(routers.Get(22), routers.Get(23));
	routerToRouter[40] = NodeContainer(routers.Get(22), routers.Get(24));
	routerToRouter[41] = NodeContainer(routers.Get(23), routers.Get(24));
	routerToRouter[42] = NodeContainer(routers.Get(23), routers.Get(27));
	routerToRouter[43] = NodeContainer(routers.Get(24), routers.Get(26));
	routerToRouter[44] = NodeContainer(routers.Get(25), routers.Get(27));
	routerToRouter[45] = NodeContainer(routers.Get(25), routers.Get(28));
	routerToRouter[46] = NodeContainer(routers.Get(26), routers.Get(27));
	routerToRouter[47] = NodeContainer(routers.Get(26), routers.Get(29));
	routerToRouter[48] = NodeContainer(routers.Get(27), routers.Get(28));
	routerToRouter[49] = NodeContainer(routers.Get(28), routers.Get(29));


	//install hosts protocols
	InternetStackHelper stack;
    stack.SetIpv6StackInstall (false);
	stack.Install(hosts);

	//routing protocols
    RipHelper ripRouting;

    /*ripRouting.ExcludeInterface (routers.Get(0), 1);
    ripRouting.ExcludeInterface (routers.Get(0), 2);
    ripRouting.ExcludeInterface (routers.Get(1), 1);
    ripRouting.ExcludeInterface (routers.Get(1), 2);
    ripRouting.ExcludeInterface (routers.Get(2), 1);
    ripRouting.ExcludeInterface (routers.Get(2), 2);
    ripRouting.ExcludeInterface (routers.Get(3), 1);
    ripRouting.ExcludeInterface (routers.Get(3), 2);
    ripRouting.ExcludeInterface (routers.Get(4), 1);
    ripRouting.ExcludeInterface (routers.Get(4), 1);
    ripRouting.ExcludeInterface (routers.Get(5), 1);
    ripRouting.ExcludeInterface (routers.Get(7), 1);
    ripRouting.ExcludeInterface (routers.Get(7), 2);
    ripRouting.ExcludeInterface (routers.Get(7), 3);
    ripRouting.ExcludeInterface (routers.Get(8), 1);
    ripRouting.ExcludeInterface (routers.Get(8), 2);
    ripRouting.ExcludeInterface (routers.Get(8), 3);
    ripRouting.ExcludeInterface (routers.Get(9), 1);
    ripRouting.ExcludeInterface (routers.Get(10), 1);
    ripRouting.ExcludeInterface (routers.Get(10), 2);
    ripRouting.ExcludeInterface (routers.Get(11), 1);
    ripRouting.ExcludeInterface (routers.Get(12), 1);
    ripRouting.ExcludeInterface (routers.Get(12), 2);
    ripRouting.ExcludeInterface (routers.Get(13), 1);
    ripRouting.ExcludeInterface (routers.Get(13), 2);
    ripRouting.ExcludeInterface (routers.Get(14), 1);
    ripRouting.ExcludeInterface (routers.Get(15), 1);
    ripRouting.ExcludeInterface (routers.Get(15), 2);
    ripRouting.ExcludeInterface (routers.Get(15), 3);
    ripRouting.ExcludeInterface (routers.Get(16), 1);
    ripRouting.ExcludeInterface (routers.Get(16), 2);
    ripRouting.ExcludeInterface (routers.Get(16), 3);
    ripRouting.ExcludeInterface (routers.Get(17), 1);
    ripRouting.ExcludeInterface (routers.Get(18), 1);
    ripRouting.ExcludeInterface (routers.Get(19), 1);
    ripRouting.ExcludeInterface (routers.Get(20), 1);
    ripRouting.ExcludeInterface (routers.Get(20), 2);
    ripRouting.ExcludeInterface (routers.Get(20), 3);
    ripRouting.ExcludeInterface (routers.Get(20), 4);
    ripRouting.ExcludeInterface (routers.Get(21), 1);
    ripRouting.ExcludeInterface (routers.Get(23), 1);
    ripRouting.ExcludeInterface (routers.Get(24), 1);
    ripRouting.ExcludeInterface (routers.Get(25), 1);
    ripRouting.ExcludeInterface (routers.Get(26), 1);
    ripRouting.ExcludeInterface (routers.Get(27), 1);
    ripRouting.ExcludeInterface (routers.Get(28), 1);
    ripRouting.ExcludeInterface (routers.Get(28), 2);
    ripRouting.ExcludeInterface (routers.Get(28), 3);
    ripRouting.ExcludeInterface (routers.Get(29), 1);
    ripRouting.ExcludeInterface (routers.Get(29), 2);
    ripRouting.ExcludeInterface (routers.Get(29), 3);*/

    Ipv4ListRoutingHelper listRH;
    listRH.Add (ripRouting, 0);

    InternetStackHelper internetRouters;
    internetRouters.SetIpv6StackInstall (false);
    internetRouters.SetRoutingHelper (listRH);
    internetRouters.Install (routers);

	// set channel attributes
	//NS_LOG_UNCOND("Create Channels.");
	//PointToPointHelper pointToPoint;
	
	pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
	pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));
	 pointToPoint.SetQueue("ns3::DropTailQueue", "MaxPackets", StringValue("1000000"));
	 pointToPoint.SetDeviceAttribute("InterframeGap", StringValue("333333ns"));
	// install net device and channels

	for (uint32_t i = 0; i < 50; i++)
	{
		devicesHosts[i] = pointToPoint.Install(routerToHost[i]);
	}
	pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
	pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));
	pointToPoint.SetQueue("ns3::DropTailQueue", "MaxPackets", StringValue("1000000"));
	pointToPoint.SetDeviceAttribute("InterframeGap", StringValue("333333ns"));
	for (uint32_t i = 0; i < 50; i++)
	{
		devicesRouters[i] = pointToPoint.Install(routerToRouter[i]);
	}

	// set false rate to each Hosts
	Ptr<RateErrorModel> em = CreateObject<RateErrorModel>();
	em->SetAttribute("ErrorRate", DoubleValue(0.0001));
	for (uint32_t i = 0; i < 50; i++)
	{
		devicesHosts[i].Get(1)->SetAttribute("ReceiveErrorModel", PointerValue(em));
	}

	//assign IP address to decices
	//NS_LOG_UNCOND("Assign IP Addresses.");
	Ipv4AddressHelper addressHosts;
	vector<Ipv4InterfaceContainer> interfacesHosts(50);
	addressHosts.SetBase("140.1.1.0", "255.255.255.0");
	interfacesHosts[0] = addressHosts.Assign(devicesHosts[0]);
	interfacesHosts[1] = addressHosts.Assign(devicesHosts[1]);
	addressHosts.SetBase("140.1.2.0", "255.255.255.0");
	interfacesHosts[2] = addressHosts.Assign(devicesHosts[2]);
	interfacesHosts[3] = addressHosts.Assign(devicesHosts[3]);
	addressHosts.SetBase("140.1.3.0", "255.255.255.0");
	interfacesHosts[4] = addressHosts.Assign(devicesHosts[4]);
	interfacesHosts[5] = addressHosts.Assign(devicesHosts[5]);
	addressHosts.SetBase("140.1.4.0", "255.255.255.0");
	interfacesHosts[6] = addressHosts.Assign(devicesHosts[6]);
	interfacesHosts[7] = addressHosts.Assign(devicesHosts[7]);
	addressHosts.SetBase("140.1.5.0", "255.255.255.0");
	interfacesHosts[8] = addressHosts.Assign(devicesHosts[8]);
	interfacesHosts[9] = addressHosts.Assign(devicesHosts[9]);
	addressHosts.SetBase("140.1.6.0", "255.255.255.0");
	interfacesHosts[10] = addressHosts.Assign(devicesHosts[10]);
	addressHosts.SetBase("140.1.8.0", "255.255.255.0");
	interfacesHosts[11] = addressHosts.Assign(devicesHosts[11]);
	interfacesHosts[12] = addressHosts.Assign(devicesHosts[12]);
	interfacesHosts[13] = addressHosts.Assign(devicesHosts[13]);
	addressHosts.SetBase("140.1.9.0", "255.255.255.0");
	interfacesHosts[14] = addressHosts.Assign(devicesHosts[14]);
	interfacesHosts[15] = addressHosts.Assign(devicesHosts[15]);
	interfacesHosts[16] = addressHosts.Assign(devicesHosts[16]);
	addressHosts.SetBase("140.1.10.0", "255.255.255.0");
	interfacesHosts[18] = addressHosts.Assign(devicesHosts[18]);
	addressHosts.SetBase("140.1.11.0", "255.255.255.0");
	interfacesHosts[19] = addressHosts.Assign(devicesHosts[19]);
	interfacesHosts[20] = addressHosts.Assign(devicesHosts[20]);
	addressHosts.SetBase("140.1.12.0", "255.255.255.0");
	interfacesHosts[21] = addressHosts.Assign(devicesHosts[21]);
	addressHosts.SetBase("140.1.13.0", "255.255.255.0");
	interfacesHosts[17] = addressHosts.Assign(devicesHosts[17]);
	addressHosts.SetBase("140.1.14.0", "255.255.255.0");
	interfacesHosts[22] = addressHosts.Assign(devicesHosts[22]);
	interfacesHosts[23] = addressHosts.Assign(devicesHosts[23]);
	addressHosts.SetBase("140.1.15.0", "255.255.255.0");
	interfacesHosts[24] = addressHosts.Assign(devicesHosts[24]);
	addressHosts.SetBase("140.1.16.0", "255.255.255.0");
	interfacesHosts[25] = addressHosts.Assign(devicesHosts[25]);
	interfacesHosts[26] = addressHosts.Assign(devicesHosts[26]);
	interfacesHosts[27] = addressHosts.Assign(devicesHosts[27]);
	addressHosts.SetBase("140.1.17.0", "255.255.255.0");
	interfacesHosts[28] = addressHosts.Assign(devicesHosts[28]);
	interfacesHosts[29] = addressHosts.Assign(devicesHosts[29]);
	interfacesHosts[30] = addressHosts.Assign(devicesHosts[30]);
	addressHosts.SetBase("140.1.18.0", "255.255.255.0");
	interfacesHosts[31] = addressHosts.Assign(devicesHosts[31]);
	addressHosts.SetBase("140.1.19.0", "255.255.255.0");
	interfacesHosts[32] = addressHosts.Assign(devicesHosts[32]);
	addressHosts.SetBase("140.1.20.0", "255.255.255.0");
	interfacesHosts[33] = addressHosts.Assign(devicesHosts[33]);
	addressHosts.SetBase("140.1.21.0", "255.255.255.0");
	interfacesHosts[34] = addressHosts.Assign(devicesHosts[34]);
	interfacesHosts[35] = addressHosts.Assign(devicesHosts[35]);
	interfacesHosts[36] = addressHosts.Assign(devicesHosts[36]);
	interfacesHosts[37] = addressHosts.Assign(devicesHosts[37]);
	addressHosts.SetBase("140.1.22.0", "255.255.255.0");
	interfacesHosts[38] = addressHosts.Assign(devicesHosts[38]);
	addressHosts.SetBase("140.1.24.0", "255.255.255.0");
	interfacesHosts[47] = addressHosts.Assign(devicesHosts[47]);
	addressHosts.SetBase("140.1.25.0", "255.255.255.0");
	interfacesHosts[49] = addressHosts.Assign(devicesHosts[49]);
	addressHosts.SetBase("140.1.26.0", "255.255.255.0");
	interfacesHosts[39] = addressHosts.Assign(devicesHosts[39]);
	addressHosts.SetBase("140.1.27.0", "255.255.255.0");
	interfacesHosts[48] = addressHosts.Assign(devicesHosts[48]);
	addressHosts.SetBase("140.1.28.0", "255.255.255.0");
	interfacesHosts[43] = addressHosts.Assign(devicesHosts[43]);
	addressHosts.SetBase("140.1.29.0", "255.255.255.0");
	interfacesHosts[44] = addressHosts.Assign(devicesHosts[44]);
	interfacesHosts[45] = addressHosts.Assign(devicesHosts[45]);
	interfacesHosts[46] = addressHosts.Assign(devicesHosts[46]);
	addressHosts.SetBase("140.1.30.0", "255.255.255.0");
	interfacesHosts[40] = addressHosts.Assign(devicesHosts[40]);
	interfacesHosts[41] = addressHosts.Assign(devicesHosts[41]);
	interfacesHosts[42] = addressHosts.Assign(devicesHosts[42]);
	

	Ipv4AddressHelper addressRouters;
	vector<Ipv4InterfaceContainer> interfacesRouters(50);

	for (uint32_t i = 0; i < 50; i++)
	{
		ostringstream subset;
        	subset<<"140.2."<<i+1<<".0";
        	addressRouters.SetBase(subset.str().c_str (),"255.255.255.0");
		interfacesRouters[i] = addressRouters.Assign(devicesRouters[i]);
	}
   
    for(uint32_t i=0; i<50;i++)
    {
       Ptr<Ipv4StaticRouting> staticRouting;
       staticRouting = Ipv4RoutingHelper::GetRouting <Ipv4StaticRouting> (hosts.Get(i)->GetObject<Ipv4> ()->GetRoutingProtocol ());
       staticRouting->SetDefaultRoute (interfacesHosts[i].GetAddress (0), 1 );
    }

	vector<uint32_t> sourceHosts;
	vector<uint32_t> remoteHosts;
  	uint32_t tempRemote=0;
  	uint32_t tempSource=0;
  	vector<uint32_t>::iterator it;
  	vector<uint32_t>::iterator itt;

	for (uint32_t i = 0; i < 1200; i++)
	{
		hostPairs=rand();
		hostPairs=hostPairs%25+1;
		sourceHosts.clear();
		remoteHosts.clear();
		double curTime = i * 0.1;
		for (int k = 1; k <= hostPairs; k++)
		{
			do 
			{
				tempSource = rand()%50;
				it = find(sourceHosts.begin(), sourceHosts.end(), tempSource);
				itt = find(remoteHosts.begin(), remoteHosts.end(), tempSource);
			} while (it != sourceHosts.end()||itt!=remoteHosts.end());
			sourceHosts.push_back(tempSource);
			do
			{
				tempRemote = rand()%50;
				it = find(sourceHosts.begin(), sourceHosts.end(), tempRemote);
				itt = find(remoteHosts.begin(), remoteHosts.end(), tempRemote);
			} while (it != sourceHosts.end()||itt!=remoteHosts.end());
			remoteHosts.push_back(tempRemote);
			uint16_t sinkPort = 8080;
  			Address sinkAddress (InetSocketAddress (interfacesHosts[tempRemote].GetAddress (1), sinkPort));
  			PacketSinkHelper packetSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort));
  			ApplicationContainer sinkApps = packetSinkHelper.Install (hosts.Get(tempRemote));
  			sinkApps.Start (Seconds (curTime));
  			sinkApps.Stop (Seconds (curTime+0.1));
			sinkApps.Get(0)->TraceConnectWithoutContext ("Rx", MakeCallback (&configPacket));
			Ptr<Socket> ns3UdpSocket = Socket::CreateSocket (hosts.Get (tempSource), UdpSocketFactory::GetTypeId ());
  			Ptr<MyApp> app = CreateObject<MyApp> ();
  			app->Setup (ns3UdpSocket, sinkAddress, 210, 26, DataRate ("10Mbps"));
  			hosts.Get (tempSource)->AddApplication (app);
  			app->SetStartTime (Seconds (curTime));
  			app->SetStopTime (Seconds (curTime+0.1));
		}
	}
	//std::cout << "Starting simulation" <<"\n";
	for(uint32_t i = 0; i <= 120; i++)
	{
		//for(uint32_t j=0; j<100;j++)
		//Simulator::Schedule(Seconds(j*0.01+i),&getminmaxQ);
		Simulator::Schedule (Seconds (i), &calDelay);
		for(uint32_t j=0; j<10;j++)	
		Simulator::Schedule (Seconds (j*0.1+i), &calThroughput);
	}
	Simulator::Stop (Seconds (120));
	Simulator::Run();
	Simulator::Destroy ();
  
  
	return 0;
}


