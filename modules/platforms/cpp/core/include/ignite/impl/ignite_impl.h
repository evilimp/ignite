/*
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _IGNITE_IMPL_IGNITE_IMPL
#define _IGNITE_IMPL_IGNITE_IMPL

#include <ignite/common/concurrent.h>
#include <ignite/jni/java.h>
#include <ignite/common/utils.h>

#include <ignite/impl/ignite_environment.h>
#include <ignite/impl/cache/cache_impl.h>
#include <ignite/impl/transactions/transactions_impl.h>
#include <ignite/impl/cluster/cluster_group_impl.h>
#include <ignite/impl/compute/compute_impl.h>

using namespace ignite::impl::interop;
using namespace ignite::common::concurrent;
using namespace ignite::impl::binary;
using namespace ignite::binary;

namespace ignite 
{
    namespace impl 
    {
        /*
        * PlatformProcessor op codes.
        */
        struct ProcessorOp
        {
            enum Type
            {
                GET_CACHE = 1,
                CREATE_CACHE = 2,
                GET_OR_CREATE_CACHE = 3,
                GET_TRANSACTIONS = 9,
                GET_CLUSTER_GROUP = 10,
            };
        };

        /**
         * Ignite implementation.
         */
        class IGNITE_FRIEND_EXPORT IgniteImpl : private interop::InteropTarget
        {
            typedef common::concurrent::SharedPointer<IgniteEnvironment> SP_IgniteEnvironment;
            typedef common::concurrent::SharedPointer<transactions::TransactionsImpl> SP_TransactionsImpl;
            typedef common::concurrent::SharedPointer<compute::ComputeImpl> SP_ComputeImpl;
            typedef common::concurrent::SharedPointer<IgniteBindingImpl> SP_IgniteBindingImpl;
        public:
            /**
             * Constructor used to create new instance.
             *
             * @param env Environment.
             * @param javaRef Reference to java object.
             */
            IgniteImpl(SP_IgniteEnvironment env);
            
            /**
             * Get name of the Ignite.
             *
             * @return Name.
             */
            const char* GetName() const;

            /**
             * Get node configuration.
             *
             * @return Node configuration.
             */
            const IgniteConfiguration& GetConfiguration() const;

            /**
             * Get JNI context associated with this instance.
             *
             * @return JNI context for this instance.
             */
            jni::java::JniContext* GetContext();

            /**
             * Get cache.
             *
             * @param name Cache name.
             * @param err Error.
             */
            cache::CacheImpl* GetCache(const char* name, IgniteError& err)
            {
                return GetOrCreateCache(name, err, ProcessorOp::GET_CACHE);
            }

            /**
             * Get or create cache.
             *
             * @param name Cache name.
             * @param err Error.
             */
            cache::CacheImpl* GetOrCreateCache(const char* name, IgniteError& err)
            {
                return GetOrCreateCache(name, err, ProcessorOp::GET_OR_CREATE_CACHE);
            }

            /**
             * Create cache.
             *
             * @param name Cache name.
             * @param err Error.
             */
            cache::CacheImpl* CreateCache(const char* name, IgniteError& err)
            {
                return GetOrCreateCache(name, err, ProcessorOp::CREATE_CACHE);
            }

            /**
             * Get ignite binding.
             *
             * @return IgniteBinding class instance.
             */
            SP_IgniteBindingImpl GetBinding();

            /**
             * Get instance of the implementation from the proxy class.
             * Internal method. Should not be used by user.
             *
             * @param proxy Proxy instance containing IgniteImpl.
             * @return IgniteImpl instance associated with the proxy or null-pointer.
             */
            template<typename T>
            static IgniteImpl* GetFromProxy(T& proxy)
            {
                return proxy.impl.Get();
            }

            /**
             * Get environment.
             * Internal method. Should not be used by user.
             *
             * @return Environment pointer.
             */
            IgniteEnvironment* GetEnvironment()
            {
                return env.Get();
            }

            /**
             * Get transactions.
             *
             * @return TransactionsImpl instance.
             */
            SP_TransactionsImpl GetTransactions()
            {
                return txImpl;
            }

            /**
             * Get projection.
             *
             * @return ClusterGroupImpl instance.
             */
            cluster::SP_ClusterGroupImpl GetProjection()
            {
                return prjImpl;
            }

            /**
             * Get compute.
             *
             * @return ComputeImpl instance.
             */
            SP_ComputeImpl GetCompute();

        private:
            /**
             * Get transactions internal call.
             *
             * @return TransactionsImpl instance.
             */
            SP_TransactionsImpl InternalGetTransactions(IgniteError &err);

            /**
             * Get current projection internal call.
             *
             * @return ClusterGroupImpl instance.
             */
            cluster::SP_ClusterGroupImpl InternalGetProjection(IgniteError &err);

            /** Environment. */
            SP_IgniteEnvironment env;

            /** Transactions implementaion. */
            SP_TransactionsImpl txImpl;

            /** Projection implementation. */
            cluster::SP_ClusterGroupImpl prjImpl;

            IGNITE_NO_COPY_ASSIGNMENT(IgniteImpl)

            /**
            * Get or create cache.
            *
            * @param name Cache name.
            * @param err Error.
            * @param op Operation code.
            */
            cache::CacheImpl* GetOrCreateCache(const char* name, IgniteError& err, int32_t op)
            {
                SharedPointer<InteropMemory> mem = env.Get()->AllocateMemory();
                InteropMemory* mem0 = mem.Get();
                InteropOutputStream out(mem0);
                BinaryWriterImpl writer(&out, env.Get()->GetTypeManager());
                BinaryRawWriter rawWriter(&writer);

                rawWriter.WriteString(name);

                out.Synchronize();

                jobject cacheJavaRef = InStreamOutObject(op, *mem0, err);

                if (!cacheJavaRef)
                {
                    return NULL;
                }

                char* name0 = common::CopyChars(name);

                return new cache::CacheImpl(name0, env, cacheJavaRef);
            }

        };
    }
}

#endif //_IGNITE_IMPL_IGNITE_IMPL