/****************************************************************************
 * Copyright (c) 2014 - 2019 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file cbops_support.c
 * \ingroup cbops
 *
 * This file contains helper functions for creating and destroying cbops
 *  graphs
 *
 * \section sec1 Contains:
 *      cbops_alloc_graph
 *      cbops_set_io_buffer
 *      cbops_append_operator_to_graph
 *      destroy_graph
 *      cbops_get_amount_written
 */

/****************************************************************************
Include Files
 */
#include "cbops_c.h"
#include "pmalloc/pl_malloc.h"

#include "cbops_iir_resamplerv2_op.h"

cbops_graph* cbops_alloc_graph(unsigned num_io)
{
    cbops_graph *result;

    /* allocate cbops graph with num_io buffers */
    result =  (cbops_graph*)xzpmalloc(sizeof(cbops_graph) +  num_io*sizeof(cbops_buffer) );
    if(result)
    {
        /* Setup Buffers.  As invalid */
        unsigned     i;
        cbops_buffer *buffer = result->buffers;

        result->num_io = num_io;
        for(i=0;i<num_io;i++,buffer++)
        {
            buffer->type         = CBOPS_IO_INTERNAL;
            buffer->transfer_ptr = &buffer->transfer_amount;
        }
		cbops_refresh_buffers(result);
		
		cbops_set_max_ops(result, CBOPS_DEFAULT_MAX_OPERATORS);
    }
    return(result);
}


void cbops_set_max_ops(cbops_graph *graph, unsigned max_ops)
{
    graph->max_ops = max_ops;
}

inline static void cbops_set_io_buffer(cbops_graph *graph,unsigned index,unsigned share_index,tCbuffer *buf,cbops_buffer_type type)
{
   cbops_buffer *buffer = &graph->buffers[index];

   if(buf==NULL)
   {
      /* Disconnected Buffer.   Mark as internal so we don't update it */
      type = CBOPS_IO_INTERNAL;
   }
   else if(index!=share_index)
   {
      type |= CBOPS_IO_SHARED;
   }

   buffer->type         = type;
   buffer->buffer       = buf;
   buffer->transfer_ptr = &(graph->buffers[share_index].transfer_amount);

   /* Buffer change, signal refresh */
   cbops_refresh_buffers(graph);
}

void cbops_unset_buffer(cbops_graph *graph,unsigned index)
{
    cbops_buffer *buffer = &graph->buffers[index];
    if(buffer != NULL)
    {
        /* make the buffer index unused */
        buffer->type = CBOPS_IO_INTERNAL;
        buffer->buffer = NULL;
        buffer->transfer_ptr = &buffer->transfer_amount;

        /* Buffer change, signal refresh */
        cbops_refresh_buffers(graph);
    }
}

void cbops_set_input_io_buffer(cbops_graph *graph,unsigned index,unsigned share_index,tCbuffer *buf)
{
   cbops_set_io_buffer(graph,index,share_index,buf,CBOPS_IO_SOURCE);
}

void cbops_set_output_io_buffer(cbops_graph *graph,unsigned index,unsigned share_index,tCbuffer *buf)
{
   cbops_set_io_buffer(graph,index,share_index,buf,CBOPS_IO_SINK);
}

void cbops_set_internal_io_buffer(cbops_graph *graph,unsigned index,unsigned share_index,tCbuffer *buf)
{
   cbops_set_io_buffer(graph,index,share_index,buf,CBOPS_IO_INTERNAL);
}

void cbops_prepend_operator_to_graph(cbops_graph *graph,cbops_op *op)
{
   /* Add to beginning of operator list */
   op->prev_operator_addr = NULL;
   op->next_operator_addr = graph->first;
   if(graph->first)
   {
      graph->first->prev_operator_addr = op;
   }
   else
   {
      graph->last = op;
   }
   graph->first = op;
}

void cbops_append_operator_to_graph(cbops_graph *graph,cbops_op *op)
{
   /* Add to end of operator list */
   op->next_operator_addr = NULL;
   op->prev_operator_addr = graph->last;
   if(graph->last)
   {
      graph->last->next_operator_addr = op;
   }
   else
   {
      graph->first = op;
   }
   graph->last = op;
}


void cbops_set_override_operator(cbops_graph *graph,cbops_op *op)
{
   cbops_functions *src_vec = op->function_vector;
   cbops_functions *dst_vec = &graph->override_funcs;

   graph->override_data   = op->parameter_area_start;
   dst_vec->reset         = src_vec->reset;
   dst_vec->amount_to_use = src_vec->amount_to_use;
   dst_vec->process       = src_vec->process;
}

static void destroy_operator(cbops_op *op)
{
    if(op->function_vector == (void*)cbops_rate_adjust_table)
    {
         destroy_sw_rate_adj_op((cbops_op*)op);
    }
    else if(op->function_vector == (void*)cbops_iir_resampler_table)
    {
         destroy_iir_resamplerv2_op(op);
    }
    else
    {
        pfree(op);
    }
}

void cbops_remove_operator_from_graph(cbops_graph *graph,cbops_op *op)
{
    cbops_op   *prev_op = op->prev_operator_addr;
    cbops_op   *next_op = op->next_operator_addr;

   if(graph->first == op)
   {
       /* we are removing the first operator in the graph,
        * next op will become new first
        */
       graph->first = next_op;
       if(next_op == NULL)
       {
           /* this op was the only op in the graph
            * now graph is empty
            */
           graph->last = NULL;
       }
       else
       {
           /* there is still op remaing,
            * just config the new first
            */
           next_op->prev_operator_addr = NULL;
       }
   }
   else if(next_op == NULL)
   {
       /* This isn't the only operator in the graph,
        * but it is the last one in chain
        */
       prev_op->next_operator_addr = NULL;
       graph->last = prev_op;
   }
   else
   {
       /* easy part, the op is neither first
        * nor last
        */
       prev_op->next_operator_addr = next_op;
       next_op->prev_operator_addr = prev_op;
   }

   /* op removed from the graph, now
    * destroy the operator
    */
   destroy_operator(op);
}

void cbops_insert_operator_into_graph(cbops_graph *graph,cbops_op *op, cbops_op *after)
{
    if(after->next_operator_addr == NULL)
    {
        /* if this is the last operator in the graph
         * then append it to end of graph
         */
        cbops_append_operator_to_graph(graph, op);
    }
    else
    {
        /* put the operator between after and after->next */
        op->prev_operator_addr = after;
        op->next_operator_addr = after->next_operator_addr;
        after->next_operator_addr->prev_operator_addr = op;        
        after->next_operator_addr = op;
    }
}

void cbops_free_operators(cbops_graph *graph)
{
    cbops_op *op,*next_op;

    /* Destroy Operators */
    if(graph->override_data)
    {
       /* Destroy override operator */
       op = (cbops_op*)( ((uintptr_t)graph->override_data) - sizeof(cbops_op));
       destroy_operator(op);
       /* Clear fields */
       graph->override_data = NULL;
       graph->override_funcs.reset         = NULL;
       graph->override_funcs.amount_to_use = NULL;
       graph->override_funcs.process       = NULL;
    }

    for(op = graph->first;op;op = next_op)
    {
       next_op = op->next_operator_addr;
       destroy_operator(op);
    }

    graph->first = graph->last = NULL;
}

void destroy_graph(cbops_graph *graph)
{
    cbops_op *op,*next_op;

    /* Destroy Operators */
    if(graph->override_data)
    {
       /* Destroy override operator */
       op = (cbops_op*)( ((uintptr_t)graph->override_data) - sizeof(cbops_op));
       destroy_operator(op);
    }

    for(op = graph->first;op;op = next_op)
    {
       next_op = op->next_operator_addr;
       destroy_operator(op);
    }

    /* Destroy Graph */
    pfree(graph);
}

/* Set buffer table index of input channel "n" */
bool cbops_set_input_idx(cbops_op* op, unsigned channel, unsigned idx)
{
    cbops_param_hdr* header = (cbops_param_hdr*)(op->parameter_area_start);

    if(channel >= header->nr_inputs)
    {
        return FALSE;
    }

    header->index_table[channel] = idx;
    return TRUE;
}

/* Set buffer table index of output channel "n" */
bool cbops_set_output_idx(cbops_op* op, unsigned channel, unsigned idx)
{
    cbops_param_hdr* header = (cbops_param_hdr*)(op->parameter_area_start);

    if(channel >= header->nr_outputs)
    {
        return FALSE;
    }

    header->index_table[header->nr_inputs + channel] = idx;
    return TRUE;
}

/* Populate cbop param struct header part */
void *cbops_populate_param_hdr(cbops_op* op,
                              unsigned nr_ins, unsigned nr_outs,
                              unsigned* input_idx, unsigned* output_idx)
{
    unsigned channel;

    /* Pointer to parameter area, this begins with cbops_param_hdr structure */
    cbops_param_hdr *params_hdr = (cbops_param_hdr*)CBOPS_OPERATOR_DATA_PTR(op);

    op->prev_operator_addr = CBOPS_NO_MORE_OPERATORS_PTR;
    op->next_operator_addr = CBOPS_NO_MORE_OPERATORS_PTR;

    params_hdr->nr_inputs = nr_ins;
    params_hdr->nr_outputs = nr_outs;

    params_hdr->operator_data_ptr = (void*)&params_hdr->index_table[nr_ins+nr_outs];

    if(input_idx != NULL)
    {
        for(channel=0; channel < nr_ins; channel++)
        {
            params_hdr->index_table[channel] = input_idx[channel];
        }
    }

    if(output_idx != NULL)
    {
        for(channel=0; channel < nr_outs; channel++)
        {
            params_hdr->index_table[nr_ins + channel] = output_idx[channel];
        }
    }

    return(params_hdr->operator_data_ptr);
}

/* Create and populate a default index vector */
unsigned* create_default_indexes(unsigned nr_io)
{
    unsigned i;
    unsigned* idx = xpnewn(nr_io, unsigned);

    if(idx != NULL)
    {
        /* Inputs are 0...nr_ins in the table... */
        /* ... followed by buffer indexes nr_ins + (0...nr_outs-1) */
        for(i=0; i < nr_io; i++)
        {
            idx[i] = i;
        }
    }

    return idx;
}

/*
 * cbops_get_amount_written
 */
unsigned cbops_get_amount(cbops_graph* graph, unsigned index)
{
    if (index < graph->num_io)
    {
        return *(graph->buffers[index].transfer_ptr);
    }
    /* Not a valid buffer index return 0 */
    return 0;
}

