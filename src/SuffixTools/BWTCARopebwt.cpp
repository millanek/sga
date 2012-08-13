//-----------------------------------------------
// Copyright 2012 Wellcome Trust Sanger Institute
// Written by Jared Simpson (js18@sanger.ac.uk)
// Released under the GPL
//-----------------------------------------------
//
// BWTCARopebwt - Construct the BWT for a set of reads
// using Heng Li's ropebwt implementation
//
#include "BWTCARopebwt.h"
#include "bcr.h"
#include "SeqReader.h"
#include "StdAlnTools.h"
#include "BWTWriterBinary.h"
#include "BWTWriterAscii.h"
#include "SAWriter.h"

static unsigned char seq_nt6_table[128] = {
    0, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    5, 1, 5, 2, 5, 5, 5, 3, 5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 5, 5, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    5, 1, 5, 2, 5, 5, 5, 3, 5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 5, 5, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5
};

void BWTCA::runRopebwt(const std::string& input_filename)
{
    // Initialize ropebwt
    bcr_t* bcr = bcr_init(true, "ropebwt-tmp");

    size_t num_sequences = 0;
    size_t num_bases = 0;
    SeqReader reader(input_filename);
    SeqRecord record;
    while(reader.get(record))
    {
        size_t l = record.seq.length();

        // Convert the string into the alphabet encoding expected by ropebwt
        uint8_t* s = new uint8_t[l];
        for(size_t i = 0; i < l; ++i) {
            char c = record.seq.get(i);
            s[i] = seq_nt6_table[(int)c];
        }
        
        // Send the sequence to ropebwt
        bcr_append(bcr, l, s);

        num_sequences += 1;
        num_bases += l;
        delete [] s;
    }

    // Build the BWT
    bcr_build(bcr);
    
    // write the BWT and SAI
    bcritr_t* itr = bcr_itr_init(bcr);
    const uint8_t* s;
    int l;

    BWTWriterBinary out_bwt("testrope.bwt");
    SAWriter out_sai("testrope.sai");

    // Write headers
    out_sai.writeHeader(num_sequences, num_sequences);
    size_t num_symbols = num_bases + num_sequences;
    out_bwt.writeHeader(num_sequences, num_symbols, BWF_NOFMI);

    // Write each run
    size_t lexo_seq_idx = 0;
    while( (s = bcr_itr_next(itr, &l)) != 0 ) {
        for (int i = 0; i < l; ++i) {
            char c = "$ACGTN"[s[i]&7];
            int rl = s[i]>>3;
            for(int j = 0; j < rl; ++j)
                out_bwt.writeBWChar(c);

            if(c == '$') {
                size_t pos_index = bcr_getLexicographicIndex(bcr, lexo_seq_idx++);
                out_sai.writeElem(SAElem(pos_index, 0));
            }
        }
    }

    free(itr);
    out_bwt.finalize();

    // Cleanup
    bcr_destroy(bcr);
}
