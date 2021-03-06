/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */
#pragma once

#include "AzCore/std/typetraits/aligned_storage.h"
#include "AzCore/std/typetraits/alignment_of.h"
#include "AzCore/std/typetraits/is_object.h"
#include "AzCore/std/typetraits/is_void.h"
#include "AzCore/std/typetraits/decay.h"
#include "AzCore/std/utils.h"

namespace AZ
{
    namespace Internal
    {
        // Shorthand for:
        // if Type != void:
        //   Type
        // else:
        //   Don't instantiate
        template <class Type>
        using enable_if_not_void = typename AZStd::enable_if<AZStd::is_void<Type>::value == false, Type>::type;

        // Shorthand for:
        // if is_valid(new Type(ArgsT...)):
        //   Type
        // else:
        //   Don't instantiate
        template <class Type, class ... ArgsT>
        using enable_if_constructible = typename AZStd::enable_if<AZStd::is_constructible<Type, ArgsT...>::value, Type>::type;

        //! Storage for Outcome's Success and Failure types.
        //! A specialization of the OutcomeStorage class handles void types.
        template <class ValueT, AZ::u8 instantiationNum>
        struct OutcomeStorage
        {
            using ValueType = ValueT;

            AZ_STATIC_ASSERT(AZStd::is_object<ValueType>::value,
                "Cannot instantiate Outcome using non-object type (except for void).");

            OutcomeStorage() = delete;

            OutcomeStorage(const ValueType& value)
                : m_value(value)
            {}

#ifdef AZ_HAS_RVALUE_REFS
            OutcomeStorage(ValueType&& value)
                : m_value(AZStd::move(value))
            {}
#endif // AZ_HAS_RVALUE_REFS

            OutcomeStorage(const OutcomeStorage& other)
                : m_value(other.m_value)
            {}

#ifdef AZ_HAS_RVALUE_REFS
            OutcomeStorage(OutcomeStorage&& other)
                : m_value(AZStd::move(other.m_value))
            {}
#endif // AZ_HAS_RVALUE_REFS

            OutcomeStorage& operator=(const OutcomeStorage& other)
            {
                m_value = other.m_value;
                return *this;
            }

#ifdef AZ_HAS_RVALUE_REFS
            OutcomeStorage& operator=(OutcomeStorage&& other)
            {
                m_value = AZStd::move(other.m_value);
                return *this;
            }
#endif // AZ_HAS_RVALUE_REFS

            ValueType m_value;
        };

        //! Specialization of OutcomeStorage for void types.
        template <AZ::u8 instantiationNum>
        struct OutcomeStorage<void, instantiationNum>
        {
        };
    } // namespace Internal


    //////////////////////////////////////////////////////////////////////////
    // Aliases

    //! Aliases for usage with AZ::Success() and AZ::Failure()
    template <class ValueT>
    using SuccessValue = Internal::OutcomeStorage<typename AZStd::decay<ValueT>::type, 0u>;
    template <class ValueT>
    using FailureValue = Internal::OutcomeStorage<typename AZStd::decay<ValueT>::type, 1u>;
} // namespace AZ
