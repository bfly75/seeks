/**
 * This is the p2p messaging component of the Seeks project,
 * a collaborative websearch overlay network.
 *
 * Copyright (C) 2010  Emmanuel Benazera, juban@free.fr
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "SGNode.h"
#include "l2_protob_rpc_server.h"
#include "l2_protob_rpc_client.h"

namespace dht
{
   std::string SGNode::_sg_config_filename = "";
   
   SGNode::SGNode(const char *net_addr, const short &net_port,
		  const bool &generate_vnodes)
     :DHTNode(net_addr,net_port,generate_vnodes,false)
     {
	if (SGNode::_sg_config_filename.empty())
	  {
	     SGNode::_sg_config_filename = DHTNode::_dht_config_filename;
	  }
	
	if (!sg_configuration::_sg_config)
	  sg_configuration::_sg_config = new sg_configuration(SGNode::_sg_config_filename);
	
	/* check whether our search groups are in sync with our virtual nodes. */
	if (!_has_persistent_data) // set in DHTNode constructor.
	  reset_vnodes_dependent(); // resets data that is dependent on virtual nodes.
	
	/* init server. */
	init_server();
	_l1_server->run_thread();
	
	/* init sweeper. */
	sg_sweeper::init(&_sgmanager);
     }
   
   SGNode::~SGNode()
     {
     }
   
   void SGNode::init_server()
     {
	_l1_server = new l2_protob_rpc_server(_l1_na.getNetAddress(),_l1_na.getPort(),this);
	_l1_client = new l2_protob_rpc_client(); 
     }
   
   void SGNode::reset_vnodes_dependent()
     {
	// resets searchgroup data, as it is dependent on the virtual nodes.
	_sgmanager.clear_sg_db();
     }
      
   dht_err SGNode::RPC_subscribe_cb(const DHTKey &recipientKey,
				    const NetAddress &recipient,
				    const DHTKey &senderKey,
				    const NetAddress &sender,
				    const DHTKey &sgKey,
				    std::vector<Subscriber*> &peers,
				    int &status)
     {
	// fill up the peers list.
	// subscribe or not.
	// trigger a sweep (condition to alleviate the load ?)
     	
	/* check on parameters. */
	if (sgKey.count() == 0)
	  {
	     status = DHT_ERR_UNSPECIFIED_SEARCHGROUP;
	     return status;
	  }
		
	/* check on subscription, i.e. if a sender address is specified. */
	bool subscribe = false;
	if (!sender.empty())
	  subscribe = true;
	
	/* find / create searchgroup. */
	Searchgroup *sg = _sgmanager.find_load_or_create_sg(&sgKey);
	if (!sg)
	  {
	     status = DHT_ERR_UNKNOWN_PEER; // XXX: should never happen, could use another error code.
	     return status;
	  }
		
	/* select peers. */
	if ((int)sg->_vec_subscribers.size() > sg_configuration::_sg_config->_max_returned_peers)
	  sg->random_peer_selection(sg_configuration::_sg_config->_max_returned_peers,peers);
	else peers = sg->_vec_subscribers;
	
	/* subscription. */
	if (subscribe)
	  {
	     Subscriber *nsub = new Subscriber(senderKey,
					       sender.getNetAddress(),sender.getPort());
	     if (!sg->add_subscriber(nsub))
	       delete nsub;
	  }
	
	/* update usage. */
	sg->set_last_time_of_use();
	
	/* trigger a call to sweep (from sg_manager). */
	_sgmanager._sgsw.sweep();
	
	return DHT_ERR_OK;
     }

   dht_err SGNode::RPC_replicate_cb(const DHTKey &recipientKey,
				    const NetAddress &recipient,
				    const DHTKey &senderKey,
				    const NetAddress &sender,
				    const DHTKey &ownerKey,
				    const std::vector<Searchgroup*> &sgs,
				    const bool &sdiff,
				    int &status)
     {
	//TODO: verify that sender address is non empty (empty means either not divulged, 
	// either fake, eliminated by server).
	//TODO: if no peer in searchgroup, simply update the replication radius.
	
     }
   
} /* end of namespace. */