/*
    Copyright 2013 Sébastien Boisvert
    Copyright 2013 Université Laval
    Copyright 2013 Centre Hospitalier Universitaire de Québec

    This file is part of Ray Surveyor.

    Ray Surveyor is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3 of the License.

    Ray Surveyor is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Ray Surveyor.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "StoreKeeper.h"
#include "CoalescenceManager.h"

#include <code/VerticesExtractor/Vertex.h>

#include <iostream>
using namespace std;

#include <string.h>

StoreKeeper::StoreKeeper() {

	m_receivedObjects = 0;

	m_configured = false;
	m_kmerLength = 0;
}

StoreKeeper::~StoreKeeper() {

}

void StoreKeeper::receive(Message & message) {

	int tag = message.getTag();

	if(!m_configured)
		configureHashTable();

	if(tag == PUSH_SAMPLE_VERTEX) {
		pushSampleVertex(message);

	} else if( tag == CoalescenceManager::DIE) {

		printName();
		cout << "(StoreKeeper) received " << m_receivedObjects << " objects in total" << endl;

		// * 2 because we store pairs
		uint64_t size = m_hashTable.size() * 2;

		printName();
		cout << "has " << size << " Kmer objects in MyHashTable instance (final)" << endl;


		printName();
		cout << "will now die (StoreKeeper)" << endl;

		die();

	} else if(CoalescenceManager::SET_KMER_LENGTH) {

		int kmerLength = 0;
		int position = 0;
		char * buffer = (char*)message.getBufferBytes();
		memcpy(&kmerLength, buffer + position, sizeof(kmerLength));
		position += sizeof(kmerLength);

		if(m_kmerLength == 0)
			m_kmerLength = kmerLength;

		// cout << "DEBUG m_kmerLength = " << m_kmerLength << endl;

		// the color space mode is an artefact.
		m_colorSpaceMode = false;

#if 0
		cout << "DEBUG StoreKeeper SET_KMER_LENGTH ";
		cout << m_kmerLength;
		cout << endl;
#endif

		/*
		memcpy(&m_parameters, buffer + position, sizeof(m_parameters));
		position += sizeof(m_parameters);

		*/
		//configureHashTable();

	}
}

void StoreKeeper::configureHashTable() {

	uint64_t buckets = 268435456;

	int bucketsPerGroup = 32 + 16 + 8 + 8;

	// \see http://docs.oracle.com/javase/7/docs/api/java/util/HashMap.html
	double loadFactorThreshold = 0.75;

	int rank = getRank();

	bool showMemoryAllocation = false;

	m_hashTable.constructor(buckets,"/apps/Ray-Surveyor/actors/StoreKeeper.txt",
		showMemoryAllocation, rank,
		bucketsPerGroup,loadFactorThreshold
		);

	m_configured = true;
}

void StoreKeeper::pushSampleVertex(Message & message) {
	char * buffer = (char*)message.getBufferBytes();
	int bytes = message.getNumberOfBytes();

	int position = 0;

	int producer = -1;
	bytes -= sizeof(producer);
	memcpy(&producer, buffer + bytes, sizeof(producer));

	/*
	printName();
	cout << "Received payload, last producer was " << producer << endl;
	*/

	while(position < bytes) {
		Vertex vertex;


		position += vertex.load(buffer + position);

		int sample = -1;
		memcpy(&sample, buffer + position, sizeof(sample));
		position += sizeof(sample);

		storeData(vertex, sample);

	/*
		printName();
		cout << " DEBUG received ";
		cout << "(from " << message.getSourceActor();
		cout << ") ";
		cout << "vertex for sample " << sample;
		cout << " with sequence ";
		vertex.print(m_kmerLength, m_colorSpaceMode);
		cout << endl;
		*/

		m_receivedObjects ++;

		if(m_receivedObjects % 1000000 == 0) {

			printStatus();
		}
	}

	int source = message.getSourceActor();

	Message response;
	response.setTag(PUSH_SAMPLE_VERTEX_OK);
	response.setBuffer(&producer);
	response.setNumberOfBytes(sizeof(producer));

	send(source, response);
}

void StoreKeeper::printStatus() {

	printName();
	cout << "(StoreKeeper) received " << m_receivedObjects << " objects so far !" << endl;
}

void StoreKeeper::storeData(Vertex & vertex, int & sample) {

	Kmer kmer = vertex.getKey();

	Kmer lowerKey = kmer.complementVertex(m_kmerLength, m_colorSpaceMode);

	if(kmer < lowerKey){
		lowerKey= kmer;
	}

	m_hashTable.insert(&lowerKey);

	// * 2 because we store pairs
	uint64_t size = m_hashTable.size() * 2;

	int period = 1000000;
	if(size % period == 0 && size != m_lastSize) {

		printName();
		cout << "has " << size << " Kmer objects in MyHashTable instance" << endl;

		m_lastSize = size;
	}
}
