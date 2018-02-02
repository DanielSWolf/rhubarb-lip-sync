/////////////////////////////////////////////////////////////////////////////// 
// 
// Copyright (c) 2015 Microsoft Corporation. All rights reserved. 
// 
// This code is licensed under the MIT License (MIT). 
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN 
// THE SOFTWARE. 
// 
///////////////////////////////////////////////////////////////////////////////

#include <UnitTest++/UnitTest++.h> 
#include <span.h>

#include <string>
#include <vector>
#include <list>
#include <iostream>
#include <memory>
#include <map>

using namespace std;
using namespace gsl;

namespace 
{
	struct BaseClass {};
	struct DerivedClass : BaseClass {};
}

SUITE(strided_span_tests)
{
	TEST (span_section_test)
	{
		int a[30][4][5];
		
		auto av = as_span(a);
		auto sub = av.section({15, 0, 0}, gsl::index<3>{2, 2, 2});
		auto subsub = sub.section({1, 0, 0}, gsl::index<3>{1, 1, 1});
		(void)subsub;
	}

	TEST(span_section)
	{
		std::vector<int> data(5 * 10);
		std::iota(begin(data), end(data), 0);
        const span<int, 5, 10> av = as_span(span<int>{data}, dim<5>(), dim<10>());

		strided_span<int, 2> av_section_1 = av.section({ 1, 2 }, { 3, 4 });
		CHECK((av_section_1[{0, 0}] == 12));
		CHECK((av_section_1[{0, 1}] == 13));
		CHECK((av_section_1[{1, 0}] == 22));
		CHECK((av_section_1[{2, 3}] == 35));

		strided_span<int, 2> av_section_2 = av_section_1.section({ 1, 2 }, { 2,2 });
		CHECK((av_section_2[{0, 0}] == 24));
		CHECK((av_section_2[{0, 1}] == 25));
		CHECK((av_section_2[{1, 0}] == 34));
	}

	TEST(strided_span_constructors)
	{
		// Check stride constructor
		{
			int arr[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
			const int carr[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };

			strided_span<int, 1> sav1{ arr, {{9}, {1}} }; // T -> T
			CHECK(sav1.bounds().index_bounds() == index<1>{ 9 });
			CHECK(sav1.bounds().stride() == 1);
			CHECK(sav1[0] == 1 && sav1[8] == 9);


			strided_span<const int, 1> sav2{ carr, {{ 4 }, { 2 }} }; // const T -> const T
			CHECK(sav2.bounds().index_bounds() == index<1>{ 4 });
			CHECK(sav2.bounds().strides() == index<1>{2});
			CHECK(sav2[0] == 1 && sav2[3] == 7);

			strided_span<int, 2> sav3{ arr, {{ 2, 2 },{ 6, 2 }} }; // T -> const T
			CHECK((sav3.bounds().index_bounds() == index<2>{ 2, 2 }));
			CHECK((sav3.bounds().strides() == index<2>{ 6, 2 }));
			CHECK((sav3[{0, 0}] == 1 && sav3[{0, 1}] == 3 && sav3[{1, 0}] == 7));
		}

		// Check span constructor
		{
			int arr[] = { 1, 2 };

			// From non-cv-qualified source
			{
				const span<int> src = arr;

				strided_span<int, 1> sav{ src, {2, 1} };
				CHECK(sav.bounds().index_bounds() == index<1>{ 2 });
				CHECK(sav.bounds().strides() == index<1>{ 1 });
				CHECK(sav[1] == 2);

#if _MSC_VER > 1800
				//strided_span<const int, 1> sav_c{ {src}, {2, 1} };
				strided_span<const int, 1> sav_c{ span<const int>{src}, strided_bounds<1>{2, 1} };
#else
				strided_span<const int, 1> sav_c{ span<const int>{src}, strided_bounds<1>{2, 1} };
#endif
				CHECK(sav_c.bounds().index_bounds() == index<1>{ 2 });
				CHECK(sav_c.bounds().strides() == index<1>{ 1 });
				CHECK(sav_c[1] == 2);

#if _MSC_VER > 1800
				strided_span<volatile int, 1> sav_v{ src, {2, 1} };
#else
				strided_span<volatile int, 1> sav_v{ span<volatile int>{src}, strided_bounds<1>{2, 1} };
#endif
				CHECK(sav_v.bounds().index_bounds() == index<1>{ 2 });
				CHECK(sav_v.bounds().strides() == index<1>{ 1 });
				CHECK(sav_v[1] == 2);

#if _MSC_VER > 1800
				strided_span<const volatile int, 1> sav_cv{ src, {2, 1} };
#else
				strided_span<const volatile int, 1> sav_cv{ span<const volatile int>{src}, strided_bounds<1>{2, 1} };
#endif
				CHECK(sav_cv.bounds().index_bounds() == index<1>{ 2 });
				CHECK(sav_cv.bounds().strides() == index<1>{ 1 });
				CHECK(sav_cv[1] == 2);
			}

			// From const-qualified source
			{
				const span<const int> src{ arr };

				strided_span<const int, 1> sav_c{ src, {2, 1} };
				CHECK(sav_c.bounds().index_bounds() == index<1>{ 2 });
				CHECK(sav_c.bounds().strides() == index<1>{ 1 });
				CHECK(sav_c[1] == 2);

#if _MSC_VER > 1800
				strided_span<const volatile int, 1> sav_cv{ src, {2, 1} };
#else
				strided_span<const volatile int, 1> sav_cv{ span<const volatile int>{src}, strided_bounds<1>{2, 1} };
#endif
				
				CHECK(sav_cv.bounds().index_bounds() == index<1>{ 2 });
				CHECK(sav_cv.bounds().strides() == index<1>{ 1 });
				CHECK(sav_cv[1] == 2);
			}

			// From volatile-qualified source
			{
				const span<volatile int> src{ arr };

				strided_span<volatile int, 1> sav_v{ src, {2, 1} };
				CHECK(sav_v.bounds().index_bounds() == index<1>{ 2 });
				CHECK(sav_v.bounds().strides() == index<1>{ 1 });
				CHECK(sav_v[1] == 2);

#if _MSC_VER > 1800
				strided_span<const volatile int, 1> sav_cv{ src, {2, 1} };
#else
				strided_span<const volatile int, 1> sav_cv{ span<const volatile int>{src}, strided_bounds<1>{2, 1} };
#endif
				CHECK(sav_cv.bounds().index_bounds() == index<1>{ 2 });
				CHECK(sav_cv.bounds().strides() == index<1>{ 1 });
				CHECK(sav_cv[1] == 2);
			}

			// From cv-qualified source
			{
				const span<const volatile int> src{ arr };

				strided_span<const volatile int, 1> sav_cv{ src, {2, 1} };
				CHECK(sav_cv.bounds().index_bounds() == index<1>{ 2 });
				CHECK(sav_cv.bounds().strides() == index<1>{ 1 });
				CHECK(sav_cv[1] == 2);
			}
		}

		// Check const-casting constructor
		{
			int arr[2] = { 4, 5 };

			const span<int, 2> av(arr, 2);
			span<const int, 2> av2{ av };
			CHECK(av2[1] == 5);

			static_assert(std::is_convertible<const span<int, 2>, span<const int, 2>>::value, "ctor is not implicit!");
		
			const strided_span<int, 1> src{ arr, {2, 1} };
			strided_span<const int, 1> sav{ src };
			CHECK(sav.bounds().index_bounds() == index<1>{ 2 });
			CHECK(sav.bounds().stride() == 1);
			CHECK(sav[1] == 5);
			
			static_assert(std::is_convertible<const strided_span<int, 1>, strided_span<const int, 1>>::value, "ctor is not implicit!");
		}

		// Check copy constructor
		{
			int arr1[2] = { 3, 4 };
			const strided_span<int, 1> src1{ arr1, {2, 1} };
			strided_span<int, 1> sav1{ src1 };
 
			CHECK(sav1.bounds().index_bounds() == index<1>{ 2 });
			CHECK(sav1.bounds().stride() == 1);
			CHECK(sav1[0] == 3);

			int arr2[6] = { 1, 2, 3, 4, 5, 6 };
			const strided_span<const int, 2> src2{ arr2, {{ 3, 2 }, { 2, 1 }} };
			strided_span<const int, 2> sav2{ src2 };
			CHECK((sav2.bounds().index_bounds() == index<2>{ 3, 2 }));
			CHECK((sav2.bounds().strides() == index<2>{ 2, 1 }));
			CHECK((sav2[{0, 0}] == 1 && sav2[{2, 0}] == 5));
		}

		// Check const-casting assignment operator
		{
			int arr1[2] = { 1, 2 };
			int arr2[6] = { 3, 4, 5, 6, 7, 8 };

			const strided_span<int, 1> src{ arr1, {{2}, {1}} };
			strided_span<const int, 1> sav{ arr2, {{3}, {2}} };
			strided_span<const int, 1>& sav_ref = (sav = src);
			CHECK(sav.bounds().index_bounds() == index<1>{ 2 });
			CHECK(sav.bounds().strides() == index<1>{ 1 });
			CHECK(sav[0] == 1);
			CHECK(&sav_ref == &sav);
		}
		
		// Check copy assignment operator
		{
			int arr1[2] = { 3, 4 };
			int arr1b[1] = { 0 };
			const strided_span<int, 1> src1{ arr1, {2, 1} };
			strided_span<int, 1> sav1{ arr1b, {1, 1} };
			strided_span<int, 1>& sav1_ref = (sav1 = src1);
			CHECK(sav1.bounds().index_bounds() == index<1>{ 2 });
			CHECK(sav1.bounds().strides() == index<1>{ 1 });
			CHECK(sav1[0] == 3);
			CHECK(&sav1_ref == &sav1);

			const int arr2[6] = { 1, 2, 3, 4, 5, 6 };
			const int arr2b[1] = { 0 };
			const strided_span<const int, 2> src2{ arr2, {{ 3, 2 },{ 2, 1 }} };
			strided_span<const int, 2> sav2{ arr2b, {{ 1, 1 },{ 1, 1 }} };
			strided_span<const int, 2>& sav2_ref = (sav2 = src2);
			CHECK((sav2.bounds().index_bounds() == index<2>{ 3, 2 }));
			CHECK((sav2.bounds().strides() == index<2>{ 2, 1 }));
			CHECK((sav2[{0, 0}] == 1 && sav2[{2, 0}] == 5));
			CHECK(&sav2_ref == &sav2);
		}
	}

	TEST(strided_span_slice)
	{
		std::vector<int> data(5 * 10);
		std::iota(begin(data), end(data), 0);
        const span<int, 5, 10> src = as_span(span<int>{data}, dim<5>(), dim<10>());

		const strided_span<int, 2> sav{ src, {{5, 10}, {10, 1}} };
#ifdef CONFIRM_COMPILATION_ERRORS
		const strided_span<const int, 2> csav{ {src},{ { 5, 10 },{ 10, 1 } } };
#endif
		const strided_span<const int, 2> csav{ span<const int, 5, 10>{ src }, { { 5, 10 },{ 10, 1 } } };

		strided_span<int, 1> sav_sl = sav[2];
		CHECK(sav_sl[0] == 20);
		CHECK(sav_sl[9] == 29);

		strided_span<const int, 1> csav_sl = sav[3];
		CHECK(csav_sl[0] == 30);
		CHECK(csav_sl[9] == 39);

		CHECK(sav[4][0] == 40);
		CHECK(sav[4][9] == 49);
	}

	TEST(strided_span_column_major)
	{
		// strided_span may be used to accomodate more peculiar
		// use cases, such as column-major multidimensional array
		// (aka. "FORTRAN" layout).

		int cm_array[3 * 5] = {
			1, 4, 7, 10, 13,
			2, 5, 8, 11, 14,
			3, 6, 9, 12, 15
		};
		strided_span<int, 2> cm_sav{ cm_array, {{ 5, 3 },{ 1, 5 }} };

		// Accessing elements
		CHECK((cm_sav[{0, 0}] == 1));
		CHECK((cm_sav[{0, 1}] == 2));
		CHECK((cm_sav[{1, 0}] == 4));
		CHECK((cm_sav[{4, 2}] == 15));

		// Slice
		strided_span<int, 1> cm_sl = cm_sav[3];

		CHECK(cm_sl[0] == 10);
		CHECK(cm_sl[1] == 11);
		CHECK(cm_sl[2] == 12);

		// Section 
		strided_span<int, 2> cm_sec = cm_sav.section( { 2, 1 }, { 3, 2 });

		CHECK((cm_sec.bounds().index_bounds() == index<2>{3, 2}));
		CHECK((cm_sec[{0, 0}] == 8));
		CHECK((cm_sec[{0, 1}] == 9));
		CHECK((cm_sec[{1, 0}] == 11));
		CHECK((cm_sec[{2, 1}] == 15));
	}

	TEST(strided_span_bounds)
	{
		int arr[] = { 0, 1, 2, 3 };
		span<int> av(arr);

		{
			// incorrect sections
			
			CHECK_THROW(av.section(0, 0)[0], fail_fast);
			CHECK_THROW(av.section(1, 0)[0], fail_fast);
			CHECK_THROW(av.section(1, 1)[1], fail_fast);
			
			CHECK_THROW(av.section(2, 5), fail_fast);
			CHECK_THROW(av.section(5, 2), fail_fast);
			CHECK_THROW(av.section(5, 0), fail_fast);
			CHECK_THROW(av.section(0, 5), fail_fast);
			CHECK_THROW(av.section(5, 5), fail_fast);
		}

		{
			// zero stride
			strided_span<int, 1> sav{ av,{ { 4 },{} } };
			CHECK(sav[0] == 0);
			CHECK(sav[3] == 0);
			CHECK_THROW(sav[4], fail_fast);
		}

		{
			// zero extent
			strided_span<int, 1> sav{ av,{ {},{ 1 } } };
			CHECK_THROW(sav[0], fail_fast);
		}

		{
			// zero extent and stride
			strided_span<int, 1> sav{ av,{ {},{} } };
			CHECK_THROW(sav[0], fail_fast);
		}

		{
			// strided array ctor with matching strided bounds 
			strided_span<int, 1> sav{ arr,{ 4, 1 } };
			CHECK(sav.bounds().index_bounds() == index<1>{ 4 });
			CHECK(sav[3] == 3);
			CHECK_THROW(sav[4], fail_fast);
		}

		{
			// strided array ctor with smaller strided bounds 
			strided_span<int, 1> sav{ arr,{ 2, 1 } };
			CHECK(sav.bounds().index_bounds() == index<1>{ 2 });
			CHECK(sav[1] == 1);
			CHECK_THROW(sav[2], fail_fast);
		}

		{
			// strided array ctor with fitting irregular bounds 
			strided_span<int, 1> sav{ arr,{ 2, 3 } };
			CHECK(sav.bounds().index_bounds() == index<1>{ 2 });
			CHECK(sav[0] == 0);
			CHECK(sav[1] == 3);
			CHECK_THROW(sav[2], fail_fast);
		}

		{
			// bounds cross data boundaries - from static arrays
			CHECK_THROW((strided_span<int, 1> { arr, { 3, 2 } }), fail_fast);
			CHECK_THROW((strided_span<int, 1> { arr, { 3, 3 } }), fail_fast);
			CHECK_THROW((strided_span<int, 1> { arr, { 4, 5 } }), fail_fast);
			CHECK_THROW((strided_span<int, 1> { arr, { 5, 1 } }), fail_fast);
			CHECK_THROW((strided_span<int, 1> { arr, { 5, 5 } }), fail_fast);
		}

		{
			// bounds cross data boundaries - from array view
			CHECK_THROW((strided_span<int, 1> { av, { 3, 2 } }), fail_fast);
			CHECK_THROW((strided_span<int, 1> { av, { 3, 3 } }), fail_fast);
			CHECK_THROW((strided_span<int, 1> { av, { 4, 5 } }), fail_fast);
			CHECK_THROW((strided_span<int, 1> { av, { 5, 1 } }), fail_fast);
			CHECK_THROW((strided_span<int, 1> { av, { 5, 5 } }), fail_fast);
		}

		{
			// bounds cross data boundaries - from dynamic arrays
			CHECK_THROW((strided_span<int, 1> { av.data(), 4, { 3, 2 } }), fail_fast);
			CHECK_THROW((strided_span<int, 1> { av.data(), 4, { 3, 3 } }), fail_fast);
			CHECK_THROW((strided_span<int, 1> { av.data(), 4, { 4, 5 } }), fail_fast);
			CHECK_THROW((strided_span<int, 1> { av.data(), 4, { 5, 1 } }), fail_fast);
			CHECK_THROW((strided_span<int, 1> { av.data(), 4, { 5, 5 } }), fail_fast);
			CHECK_THROW((strided_span<int, 1> { av.data(), 2, { 2, 2 } }), fail_fast);
		}

#ifdef CONFIRM_COMPILATION_ERRORS
		{
			strided_span<int, 1> sav0{ av.data(), { 3, 2 } };
			strided_span<int, 1> sav1{ arr, { 1 } };
			strided_span<int, 1> sav2{ arr, { 1,1,1 } };
			strided_span<int, 1> sav3{ av, { 1 } };
			strided_span<int, 1> sav4{ av, { 1,1,1 } };
			strided_span<int, 2> sav5{ av.as_span(dim<2>(), dim<2>()), { 1 } };
			strided_span<int, 2> sav6{ av.as_span(dim<2>(), dim<2>()), { 1,1,1 } };
			strided_span<int, 2> sav7{ av.as_span(dim<2>(), dim<2>()), { { 1,1 },{ 1,1 },{ 1,1 } } };

			index<1> index{ 0, 1 };
			strided_span<int, 1> sav8{ arr,{ 1,{ 1,1 } } };
			strided_span<int, 1> sav9{ arr,{ { 1,1 },{ 1,1 } } };
			strided_span<int, 1> sav10{ av,{ 1,{ 1,1 } } };
			strided_span<int, 1> sav11{ av,{ { 1,1 },{ 1,1 } } };
			strided_span<int, 2> sav12{ av.as_span(dim<2>(), dim<2>()),{ { 1 },{ 1 } } };
			strided_span<int, 2> sav13{ av.as_span(dim<2>(), dim<2>()),{ { 1 },{ 1,1,1 } } };
			strided_span<int, 2> sav14{ av.as_span(dim<2>(), dim<2>()),{ { 1,1,1 },{ 1 } } };
		}
#endif
	}

	TEST(strided_span_type_conversion)
	{
		int arr[] = { 0, 1, 2, 3 };
		span<int> av(arr);

		{
			strided_span<int, 1> sav{ av.data(), av.size(), { av.size() / 2, 2 } };
#ifdef CONFIRM_COMPILATION_ERRORS
			strided_span<long, 1> lsav1 = sav.as_strided_span<long, 1>();
#endif
		}
		{
			strided_span<int, 1> sav{ av, { av.size() / 2, 2 } };
#ifdef CONFIRM_COMPILATION_ERRORS
			strided_span<long, 1> lsav1 = sav.as_strided_span<long, 1>();
#endif
		}

		span<const byte, dynamic_range> bytes = as_bytes(av);

		// retype strided array with regular strides - from raw data
		{
			strided_bounds<2> bounds{ { 2, bytes.size() / 4 }, { bytes.size() / 2, 1 } };
			strided_span<const byte, 2> sav2{ bytes.data(), bytes.size(), bounds };
			strided_span<const int, 2> sav3 = sav2.as_strided_span<const int>();
			CHECK(sav3[0][0] == 0);
			CHECK(sav3[1][0] == 2);
			CHECK_THROW(sav3[1][1], fail_fast);
			CHECK_THROW(sav3[0][1], fail_fast);
		}

		// retype strided array with regular strides - from span
		{
			strided_bounds<2> bounds{ { 2, bytes.size() / 4 }, { bytes.size() / 2, 1 } };
			span<const byte, 2, dynamic_range> bytes2 = as_span(bytes, dim<2>(), dim<>(bytes.size() / 2));
			strided_span<const byte, 2> sav2{ bytes2, bounds };
			strided_span<int, 2> sav3 = sav2.as_strided_span<int>();
			CHECK(sav3[0][0] == 0);
			CHECK(sav3[1][0] == 2);
			CHECK_THROW(sav3[1][1], fail_fast);
			CHECK_THROW(sav3[0][1], fail_fast);
		}

		// retype strided array with not enough elements - last dimension of the array is too small
		{
			strided_bounds<2> bounds{ { 4,2 },{ 4, 1 } };
			span<const byte, 2, dynamic_range> bytes2 = as_span(bytes, dim<2>(), dim<>(bytes.size() / 2));
			strided_span<const byte, 2> sav2{ bytes2, bounds };
			CHECK_THROW(sav2.as_strided_span<int>(), fail_fast);
		}

		// retype strided array with not enough elements - strides are too small
		{
			strided_bounds<2> bounds{ { 4,2 },{ 2, 1 } };
			span<const byte, 2, dynamic_range> bytes2 = as_span(bytes, dim<2>(), dim<>(bytes.size() / 2));
			strided_span<const byte, 2> sav2{ bytes2, bounds };
			CHECK_THROW(sav2.as_strided_span<int>(), fail_fast);
		}

		// retype strided array with not enough elements - last dimension does not divide by the new typesize
		{
			strided_bounds<2> bounds{ { 2,6 },{ 4, 1 } };
			span<const byte, 2, dynamic_range> bytes2 = as_span(bytes, dim<2>(), dim<>(bytes.size() / 2));
			strided_span<const byte, 2> sav2{ bytes2, bounds };
			CHECK_THROW(sav2.as_strided_span<int>(), fail_fast);
		}

		// retype strided array with not enough elements - strides does not divide by the new typesize
		{
			strided_bounds<2> bounds{ { 2, 1 },{ 6, 1 } };
			span<const byte, 2, dynamic_range> bytes2 = as_span(bytes, dim<2>(), dim<>(bytes.size() / 2));
			strided_span<const byte, 2> sav2{ bytes2, bounds };
			CHECK_THROW(sav2.as_strided_span<int>(), fail_fast);
		}

		// retype strided array with irregular strides - from raw data
		{
			strided_bounds<1> bounds{ bytes.size() / 2, 2 };
			strided_span<const byte, 1> sav2{ bytes.data(), bytes.size(), bounds };
			CHECK_THROW(sav2.as_strided_span<int>(), fail_fast);
		}

		// retype strided array with irregular strides - from span
		{
			strided_bounds<1> bounds{ bytes.size() / 2, 2 };
			strided_span<const byte, 1> sav2{ bytes, bounds };
			CHECK_THROW(sav2.as_strided_span<int>(), fail_fast);
		}
	}

	TEST(empty_strided_spans)
	{
		{
			span<int, 0> empty_av(nullptr);
			strided_span<int, 1> empty_sav{ empty_av, { 0, 1 } };

			CHECK(empty_sav.bounds().index_bounds() == index<1>{ 0 });
			CHECK_THROW(empty_sav[0], fail_fast);
			CHECK_THROW(empty_sav.begin()[0], fail_fast);
			CHECK_THROW(empty_sav.cbegin()[0], fail_fast);

			for (auto& v : empty_sav)
			{
                (void)v;
				CHECK(false);
			}
		}

		{
			strided_span<int, 1> empty_sav{ nullptr, 0, { 0, 1 } };

			CHECK(empty_sav.bounds().index_bounds() == index<1>{ 0 });
			CHECK_THROW(empty_sav[0], fail_fast);
			CHECK_THROW(empty_sav.begin()[0], fail_fast);
			CHECK_THROW(empty_sav.cbegin()[0], fail_fast);

			for (auto& v : empty_sav)
			{
                (void)v;
				CHECK(false);
			}
		}
	}

    void iterate_every_other_element(span<int, dynamic_range> av)
    {
        // pick every other element

        auto length = av.size() / 2;
#if _MSC_VER > 1800
        auto bounds = strided_bounds<1>({length}, {2});
#else
        auto bounds = strided_bounds<1>(index<1>{ length }, index<1>{ 2 });
#endif
        strided_span<int, 1> strided(&av.data()[1], av.size() - 1, bounds);

        CHECK(strided.size() == length);
        CHECK(strided.bounds().index_bounds()[0] == length);
        for (auto i = 0; i < strided.size(); ++i)
        {
            CHECK(strided[i] == av[2 * i + 1]);
        }

        int idx = 0;
        for (auto num : strided)
        {
            CHECK(num == av[2 * idx + 1]);
            idx++;
        }
    }

    TEST(strided_span_section_iteration)
    {
        int arr[8] = {4,0,5,1,6,2,7,3};

        // static bounds
        {
            span<int, 8> av(arr, 8);
            iterate_every_other_element(av);
        }

        // dynamic bounds
        {
            span<int, dynamic_range> av(arr, 8);
            iterate_every_other_element(av);
        }
    }

    TEST(dynamic_strided_span_section_iteration)
    {
        auto arr = new int[8];
        for (int i = 0; i < 4; ++i)
        {
            arr[2 * i] = 4 + i;
            arr[2 * i + 1] = i;
        }

        auto av = as_span(arr, 8);
        iterate_every_other_element(av);

        delete[] arr;
    }

    void iterate_second_slice(span<int, dynamic_range, dynamic_range, dynamic_range> av)
    {
        int expected[6] = {2,3,10,11,18,19};
        auto section = av.section({0,1,0}, {3,1,2});

        for (auto i = 0; i < section.extent<0>(); ++i)
        {
            for (auto j = 0; j < section.extent<1>(); ++j)
                for (auto k = 0; k < section.extent<2>(); ++k)
                {
                    auto idx = index<3>{i,j,k}; // avoid braces in the CHECK macro
                    CHECK(section[idx] == expected[2 * i + 2 * j + k]);
                }
        }

        for (auto i = 0; i < section.extent<0>(); ++i)
        {
            for (auto j = 0; j < section.extent<1>(); ++j)
                for (auto k = 0; k < section.extent<2>(); ++k)
                    CHECK(section[i][j][k] == expected[2 * i + 2 * j + k]);
        }

        int i = 0;
        for (auto num : section)
        {
            CHECK(num == expected[i]);
            i++;
        }
    }

    TEST(strided_span_section_iteration_3d)
    {
        int arr[3][4][2];
        for (auto i = 0; i < 3; ++i)
        {
            for (auto j = 0; j < 4; ++j)
                for (auto k = 0; k < 2; ++k)
                    arr[i][j][k] = 8 * i + 2 * j + k;
        }

        {
            span<int, 3, 4, 2> av = arr;
            iterate_second_slice(av);
        }
    }

    TEST(dynamic_strided_span_section_iteration_3d)
    {
        auto height = 12, width = 2;
        auto size = height * width;

        auto arr = new int[size];
        for (auto i = 0; i < size; ++i)
        {
            arr[i] = i;
        }

        {
            auto av = as_span(as_span(arr, 24), dim<3>(), dim<4>(), dim<2>());
            iterate_second_slice(av);
        }

        {
            auto av = as_span(as_span(arr, 24), dim<>(3), dim<4>(), dim<2>());
            iterate_second_slice(av);
        }

        {
            auto av = as_span(as_span(arr, 24), dim<3>(), dim<>(4), dim<2>());
            iterate_second_slice(av);
        }

        {
            auto av = as_span(as_span(arr, 24), dim<3>(), dim<4>(), dim<>(2));
            iterate_second_slice(av);
        }
        delete[] arr;
    }

    TEST(strided_span_conversion)
    {
        // get an span of 'c' values from the list of X's

        struct X { int a; int b; int c; };

        X arr[4] = {{0,1,2},{3,4,5},{6,7,8},{9,10,11}};

        int s = sizeof(int) / sizeof(byte);
        auto d2 = 3 * s;
        auto d1 = sizeof(int) * 12 / d2;

        // convert to 4x12 array of bytes
        auto av = as_span(as_bytes(as_span(arr, 4)), dim<>(d1), dim<>(d2));

        CHECK(av.bounds().index_bounds()[0] == 4);
        CHECK(av.bounds().index_bounds()[1] == 12);

        // get the last 4 columns
        auto section = av.section({0, 2 * s}, {4, s}); // { { arr[0].c[0], arr[0].c[1], arr[0].c[2], arr[0].c[3] } , { arr[1].c[0], ... } , ... }

                                                       // convert to array 4x1 array of integers
        auto cs = section.as_strided_span<int>(); // { { arr[0].c }, {arr[1].c } , ... } 

        CHECK(cs.bounds().index_bounds()[0] == 4);
        CHECK(cs.bounds().index_bounds()[1] == 1);

        // transpose to 1x4 array 
        strided_bounds<2> reverse_bounds{
            {cs.bounds().index_bounds()[1] , cs.bounds().index_bounds()[0]},
            {cs.bounds().strides()[1], cs.bounds().strides()[0]}
        };

        strided_span<int, 2> transposed{cs.data(), cs.bounds().total_size(), reverse_bounds};

        // slice to get a one-dimensional array of c's
        strided_span<int, 1> result = transposed[0];

        CHECK(result.bounds().index_bounds()[0] == 4);
        CHECK_THROW(result.bounds().index_bounds()[1], fail_fast);

        int i = 0;
        for (auto& num : result)
        {
            CHECK(num == arr[i].c);
            i++;
        }

    }
}

int main(int, const char *[])
{
	return UnitTest::RunAllTests();
}
