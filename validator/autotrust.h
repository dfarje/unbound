/*
 * validator/autotrust.h - RFC5011 trust anchor management for unbound.
 *
 * Copyright (c) 2009, NLnet Labs. All rights reserved.
 *
 * This software is open source.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 
 * Neither the name of the NLNET LABS nor the names of its contributors may
 * be used to endorse or promote products derived from this software without
 * specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * \file
 *
 * Contains autotrust definitions.
 */

#ifndef VALIDATOR_AUTOTRUST_H
#define VALIDATOR_AUTOTRUST_H
#include "util/rbtree.h"
struct val_anchors;
struct trust_anchor;
struct ub_packed_rrset_key;
struct module_env;
struct val_env;

/** Autotrust anchor states */
typedef enum {
	AUTR_STATE_START   = 0,
	AUTR_STATE_ADDPEND = 1,
	AUTR_STATE_VALID   = 2,
	AUTR_STATE_MISSING = 3,
	AUTR_STATE_REVOKED = 4,
	AUTR_STATE_REMOVED = 5
} autr_state_t;

/** 
 * Autotrust metadata for one trust anchor key.
 */
struct autr_ta {
	/** next key */
	struct autr_ta* next;
	/** the RR */
	ldns_rr* rr;
	/** last update of key */
	time_t last_change;
	/** 5011 state */
	autr_state_t s;
	/** pending count */
	uint8_t pending_count;
	/** fresh TA was seen */
	uint8_t fetched;
	/** revoked TA was seen */
	uint8_t revoked;
};

/** 
 * Autotrust metadata for a trust point.
 */
struct autr_point_data {
	/** file to store the trust point in. chrootdir already applied. */
	char* file;
	/** rbtree node for probe sort, key is struct trust_anchor */
	rbnode_t pnode;

	/** the keys */
	struct autr_ta* keys;

	/** last queried DNSKEY set */
	time_t last_queried;
	/** last successful DNSKEY set */
	time_t last_success;
	/** next probe time */
	time_t next_probe_time;

	/** when to query if !failed */
	uint32_t query_interval;
	/** when to retry if failed */
	uint32_t retry_time;

	/** how many times did it fail */
	uint8_t query_failed;
};

/** 
 * Autotrust global metadata.
 */
struct autr_global_data {
	/** rbtree of autotrust anchors sorted by next probe time */
	rbtree_t probetree;
};

/**
 * Create new global 5011 data structure.
 * @return new structure or NULL on malloc failure.
 */
struct autr_global_data* autr_global_create(void);

/**
 * Delete global 5011 data structure.
 * @param global: global autotrust state to delete.
 */
void autr_global_delete(struct autr_global_data* global);

/** probe tree compare function */
int probetree_cmp(const void* x, const void* y);

/**
 * Read autotrust file.
 * @param anchors: the anchors structure.
 * @param nm: name of the file (copied).
 * @return false on failure.
 */
int autr_read_file(struct val_anchors* anchors, const char* nm);

/**
 * Write autotrust file.
 * @param env: environment with scratch space.
 * @param tp: trust point to write.
 */
void autr_write_file(struct module_env* env, struct trust_anchor* tp);

/**
 * Delete autr anchor, deletes the autr data but does not do
 * unlinking from trees, caller does that.
 * @param tp: trust point to delete.
 */
void autr_point_delete(struct trust_anchor* tp);

/**
 * Perform autotrust processing.
 * @param env: qstate environment with the anchors structure.
 * @param ve: validator environment for verification of rrsigs.
 * @param tp: trust anchor to process.
 * @param dnskey_rrset: DNSKEY rrset probed (can be NULL if bad prime result).
 * 	allocated in a region. Has not been validated yet.
 * @return false if trust anchor was revoked completely.
 * 	Otherwise logs errors to log, does not change return value.
 */
int autr_process_prime(struct module_env* env, struct val_env* ve,
	struct trust_anchor* tp, struct ub_packed_rrset_key* dnskey_rrset);

/**
 * Debug printout of rfc5011 tracked anchors
 * @param anchors: all the anchors.
 */
void autr_debug_print(struct val_anchors* anchors);

#endif /* VALIDATOR_AUTOTRUST_H */