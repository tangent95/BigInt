#define _CRT_SECURE_NO_WARNINGS

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <climits>
#include <cstdint> //uint32_t int32_t ...


//疑点:构造函数接收*this会调用哪个 多线程应考虑要不要加互斥锁
//question:xxxMath::BigInt&为返回值的函数返回值不无变量接受时还会不会去返回,返回时要重新构造?
//可能疏忽处:大数Copy以及此类拷贝复制的函数可能忘考虑输入为0的情况
//可能疏忽处:目前Free后未将m_num设置为nullptr

namespace xxxMath
{
	using UINT_32 = std::uint32_t; //32位无符号整数
	using INT_32 = std::int32_t; //32位有符号整数
	using UINT_64 = std::uint64_t; //64位无符号整数
	using INT_64 = std::int64_t; //64位有符号整数
	template <typename ArrayType>
	inline void ArrayCopy(ArrayType* destination, ArrayType* source, xxxMath::INT_32 copySize) //模板数组复制
	{
		while (--copySize >= 0)
		{
			destination[copySize] = source[copySize];
		}
	}
	class BigInt //大数
	{
	/*private*/public: //基本数据
		xxxMath::UINT_32* m_num; //数字数组(10进制),倒序存储
		xxxMath::UINT_32 m_size; //数组长度,单位:十进制位数
		xxxMath::UINT_32 m_maxSize; //数组最大长度,单位:字节(一个十进制数占四个字节,但此处最大长度可能为非四的倍数)
		xxxMath::INT_32 m_sign; //数字符号 零为0 正数为1 负数为-1
	/*private*/public: //内存管理函数
		//下一步优化:内存池
		inline void Allocate(const xxxMath::UINT_32& arraySize) //分配1.5倍长度大小的空间
		{
			m_maxSize = (arraySize * sizeof(xxxMath::UINT_32) * 3) >> 1;
			m_num = (xxxMath::UINT_32*)malloc(m_maxSize);
		}
		inline void Reallocate(const xxxMath::UINT_32& arraySize) //重分配数组空间至1.5倍长度大小的空间
		{
			//隐含的bug:realloc分配失败时会返回nullptr,导致内存泄露
			m_maxSize = (arraySize * sizeof(xxxMath::UINT_32) * 3) >> 1;
			m_num = (xxxMath::UINT_32*)realloc(m_num, m_maxSize); //新长度大于旧长度扩大空间,原数据保留,新空间数据由上一次的数值确定
		}
		inline void Free()
		{
			free(m_num);
		}
	public: //类构造析构函数
		BigInt() //默认构造函数
		{
			m_num = nullptr;
			m_size = 0;
			m_sign = 0;
			m_maxSize = 0;
		}
		BigInt(xxxMath::INT_32 input) //整数构造函数,输入需在INT_32之内
		{
			if (!input) //输入为0,不释放空间因为这是初始化,不存在它有上一次值的可能
			{
				m_num = nullptr;
				m_size = 0;
				m_sign = 0;
				m_maxSize = 0;
				return;
			}
			else if (input < 0) //输入为负数
			{
				m_sign = -1;
				input = -input; //避免数组中出现负数元素
			}
			else //输入为正数
			{
				m_sign = 1;
			}
			Allocate(10);
			register xxxMath::UINT_32 arrayPosition(0);
			while (input) //数位分离
			{
				m_num[arrayPosition++] = input % 10;
				input /= 10;
			}
			m_size = arrayPosition;
		}
		BigInt(const xxxMath::BigInt& input) //拷贝构造函数,*this返回时会调用这个
		{
			m_size = input.m_size;
			Allocate(m_size);
			ArrayCopy(m_num, input.m_num, m_size);
			m_sign = input.m_sign;
		}
		BigInt(xxxMath::BigInt&& input) noexcept //移动构造函数,用以接受函数返回值,不能加入只有单个引用的移动构造函数
		{
			m_num = input.m_num;
			input.m_num = nullptr;
			m_size = input.m_size;
			m_sign = input.m_sign;
			m_maxSize = input.m_maxSize;
		}
		~BigInt() //析构函数
		{
			if (m_num) //避免释放掉nullptr
			{
				free(m_num);
				m_num = nullptr;
				m_maxSize = 0;
				m_size = 0;
				m_sign = 0;
				//避免有人销毁类后再输出内容
			}
		}
	public: //中间函数
		inline xxxMath::UINT_32 GetMaxLength() const //获取数组最大长度
		{
			return m_maxSize / sizeof(xxxMath::UINT_32); //可以改右移
		}
	public: //大数输入输出函数
		void OutputConsole() const
		{
			if (m_sign == 0) //为0
			{
				putchar('0');
				return;
			}
			else if (m_sign < 0) //为负数
			{
				putchar('-');
			}
			register xxxMath::INT_32 arrayPosition(m_size - 1);
			while (arrayPosition >= 0) //倒序输出
			{
				putchar(m_num[arrayPosition--] + '0');
			}
		}
		xxxMath::BigInt& Move(xxxMath::BigInt&& input) noexcept //移动大数,主要移动函数返回值
		{
			if (this != &input) //自己移给自己会释放掉自己的空间
			{
				if (m_num) //释放掉旧的空间,避免内存泄漏
				{
					Free();
				}
				m_num = input.m_num;
				input.m_num = nullptr; //避免输入的大数析构导致空间被释放 多线程访问时会不会出现访问nullptr?
				m_size = input.m_size;
				m_sign = input.m_sign;
				m_maxSize = input.m_maxSize;
			}
			return *this;
		}
		xxxMath::BigInt& Move(xxxMath::BigInt& input) //移动大数
		{
			if (this != &input) //自己移给自己会释放掉自己的空间
			{
				if (m_num) //释放掉旧的空间,避免内存泄漏
				{
					Free();
				}
				m_num = input.m_num;
				input.m_num = nullptr; //避免输入的大数析构导致空间被释放 多线程访问时会不会出现访问nullptr?
				m_size = input.m_size;
				m_sign = input.m_sign;
				m_maxSize = input.m_maxSize;
				input.m_size = 0;
				input.m_sign = 0;
				input.m_maxSize = 0; //也对input进行处理因为避免input后来又被作为数参与运算而出现nullptr
			}
			return *this;
		}
		xxxMath::BigInt Copy(const xxxMath::BigInt& input) //复制大数
		{
			if (this != &input) //自己复制自己会导致内存泄漏
			{
				if (input.IsZero())
				{
					SetZero();
					return *this;
				}
				m_size = input.m_size;
				if (m_num) //不用Reallocate而先释放空间再重新分配是因为不需要保留原先的数据,不需要扩充
				{
					if (m_maxSize < m_size) //前面已排除0的情况,此处m_maxSize有意义
					{
						Free();
						Allocate(m_size);
					}
				}
				else
				{
					Allocate(m_size);
				}
				ArrayCopy(m_num, input.m_num, m_size); //拷贝数组内容
				m_sign = input.m_sign;
				//this不为0长度本来合适便不改变最大长度;this不为0长度较短便会重分配内存,最大长度改变
			}
			return *this;
		}
		xxxMath::BigInt& Initialize(xxxMath::INT_32 input) //以32位有符号整数初始化大数
		{
			if (!input) //输入为0
			{
				if (m_num)
				{
					Free();
					m_num = nullptr;
				}
				m_size = 0; //不去掉是因为Move中没有将被移动的大数的符号与长度清零,如果初始化这种大数将导致访问nullptr的可能性
				m_sign = 0;
				m_maxSize = 0;
				return *this;
			}
			else if (input < 0) //输入为负数
			{
				m_sign = -1;
				input = -input; //避免数位分离时数组中出现负数元素
			}
			else //输入为正数
			{
				m_sign = 1;
			}
			if (m_num) //已分配空间
			{
				if (GetMaxLength() < 10) //空间大小不足,不Reallocate原因参照前面的内容
				{
					free(m_num);
					Allocate(10);
				}
			}
			else //未分配空间
			{
				Allocate(10);
			}
			register xxxMath::UINT_32 arrayPosition(0);
			while (input) //数位分离
			{
				m_num[arrayPosition++] = input % 10;
				input /= 10;
			}
			m_size = arrayPosition;
			return *this;
		}
		xxxMath::BigInt& Initialize(const char* input) //字符数组初始化大数
		{ /////////////////看到这里
			register xxxMath::INT_32 inputSize((xxxMath::UINT_32)strlen(input) - 1);
			register xxxMath::UINT_32 arrayPosition(0), realSize(0); //realSize获取真实的数组长度,避免输入一堆零导致初始化为一堆零
			//此处有代码位置变迁
			while (inputSize >= 0) //在字符数组中倒序遍历
			{
				if (input[inputSize] >= '0' && input[inputSize] <= '9') //避免将非数字的负号,空格,回车(某些函数读入字符串时会在结尾加入'\n',例如fgets)纳入数组
				{
					m_num[arrayPosition] = input[inputSize] - '0';
					if (m_num[arrayPosition++]) //如果出现非零数
					{
						realSize = arrayPosition; //记录数组的真实长度
					}
				}
				--inputSize;
			}
			m_size = realSize;
			if (!m_size) //数为0
			{
				m_maxSize = 0;
				Free();
				m_num = nullptr;
			}
			else
			{
				if (m_num) //已分配空间
				{
					if (GetMaxLength() < 10/*bug*/) //空间大小不足
					{
						free(m_num);
						Allocate(inputSize + 1);
					}
				}
				else //未分配空间
				{
					Allocate(inputSize + 1);
				}
			}
			
			
			m_sign = m_size ? (input[0] == '-' ? -1 : 1) : 0; //m_size?避免了将"-0"输入符号定为负数
			return *this;
		}
		xxxMath::BigInt Initialize(const xxxMath::BigInt& input) //大数初始化大数
		{
			return Copy(input);
		}
	public: //大数信息获取与处理函数
		inline bool IsNegative() const //判断数字是否为负数
		{
			return m_sign < 0;
		}
		inline bool IsPositive() const //判断数字是否为正数
		{
			return m_sign > 0;
		}
		inline bool IsZero() const //判断数字是否为0
		{
			return m_sign == 0;
		}
		inline void SetSign(const xxxMath::INT_32& sign) //设置大数的符号
		{
			m_sign = sign > 0 ? 1 : (sign == 0 ? 0 : -1); //避免符号设为-1,0,1以外的符号
			if (sign == 0 && m_num) //当符号为零时,大数值为0,应做处理
			{
				Free();
				m_num = nullptr;
				m_size = 0;
				m_maxSize = 0;
			}
		}
		inline void SetZero()
		{
			if (m_sign)
			{
				Free();
				m_num = nullptr;
				m_size = 0;
				m_maxSize = 0;
				m_sign = 0;
			}
		}
		inline xxxMath::UINT_32 GetFirstUInt64() const //获取数组首2位的数
		{
			return m_size > 1 ? (m_num[m_size - 1] * 10 + m_num[m_size - 2]) : (m_size == 1 ? m_num[m_size - 1] : 0);
		}
	/*
	public: //基础运算,一般不用,在一些特殊的情况使用会更快
		xxxMath::INT_32 BasicCompare(const xxxMath::BigInt& input) const //比较两数绝对值大小
		{
			if (m_size ^ input.m_size) //长度不相等
			{
				return m_size > input.m_size ? 1 : -1;
			}
			for (register xxxMath::INT_32 arrayPosition(m_size - 1); arrayPosition >= 0; --arrayPosition) //从头到尾逐位比较
			{
				if (m_num[arrayPosition] ^ input.m_num[arrayPosition]) //两数不相等
				{
					return m_num[arrayPosition] > input.m_num[arrayPosition] ? 1 : -1;
				}
			}
			return 0; //两数相等
		}
		xxxMath::INT_32 BasicCompare(const xxxMath::UINT_32& input) const //比较两数绝对值大小
		{
			if (m_size > 10) //UINT_32十进制最长10位
			{
				return 1;
			}
			xxxMath::UINT_64 thisLastTen(0); //大数末尾10位的数字
			for (register xxxMath::INT_32 arrayPosition(m_size - 1); arrayPosition >= 0; --arrayPosition)
			{
				thisLastTen *= 10;
				thisLastTen += m_num[arrayPosition];
			}
			return thisLastTen > input ? 1 : (thisLastTen == input ? 0 : -1);
		}
		xxxMath::INT_32 BasicShiftCompare(const xxxMath::BigInt& input, const xxxMath::UINT_32& shift) const //比较两数绝对值大小
		{
			if ((m_size - shift) ^ input.m_size) //长度不相等
			{
				return (m_size - shift) > input.m_size ? 1 : -1;
			}
			for (register xxxMath::INT_32 arrayPosition(input.m_size - 1); arrayPosition >= 0; --arrayPosition) //从头到尾逐位比较
			{
				if (m_num[arrayPosition + shift] ^ input.m_num[arrayPosition]) //两数不相等
				{
					return m_num[arrayPosition + shift] > input.m_num[arrayPosition] ? 1 : -1;
				}
			}
			return 0; //两数相等
		}
		xxxMath::INT_32 Compare(const xxxMath::BigInt& input) const //比较两数大小
		{
			if (m_sign ^ input.m_sign) //不同号
			{
				if (m_sign == 0)
				{
					return input.m_sign;
				}
				if (input.m_sign == 0)
				{
					return m_sign;
				}
				return m_sign > 0 ? 1 : -1;
			}
			return m_sign > 0 ? BasicCompare(input) : -BasicCompare(input); //同号比较绝对值
		}
		xxxMath::INT_32 Compare(const xxxMath::INT_32& input) const //比较两数大小
		{
			xxxMath::INT_32 inputSign = input > 0 ? 1 : (input == 0 ? 0 : -1);
			if (m_sign ^ inputSign) //不同号
			{
				if (m_sign == 0)
				{
					return inputSign;
				}
				if (inputSign == 0)
				{
					return m_sign;
				}
				return m_sign > 0 ? 1 : -1;
			}
			return m_sign > 0 ? BasicCompare(input) : -BasicCompare(input); //同号比较绝对值
		}
		
		xxxMath::BigInt BasicAdd(const xxxMath::BigInt& addend) const //加法
		{
			xxxMath::BigInt result;
			xxxMath::UINT_32 maxSize(m_size > addend.m_size ? m_size : addend.m_size);
			register xxxMath::UINT_32 arrayPosition(0), addBuffer(0);
			result.Allocate(maxSize + 1);
			while (arrayPosition <= maxSize) //等于是因为包含进位
			{
				if (arrayPosition < m_size) //加上被加数
				{
					addBuffer += m_num[arrayPosition];
				}
				if (arrayPosition < addend.m_size) //加上被减数
				{
					addBuffer += addend.m_num[arrayPosition];
				}
				result.m_num[arrayPosition] = addBuffer % 10; //获取结果
				addBuffer /= 10; //获取进位
				++arrayPosition;
			}
			result.m_size = maxSize;
			if (result.m_num[maxSize]) //如果有进位
			{
				++result.m_size;
			}
			return result;
		}
		xxxMath::BigInt BasicAdd(xxxMath::UINT_32 addend) const //加法
		{
			xxxMath::BigInt result;
			register xxxMath::UINT_32 arrayPosition(0);
			result.Allocate(m_size > 10 ? m_size : 11);
			while (addend) //加到不能加为止
			{
				if (arrayPosition < m_size)
				{
					addend += m_num[arrayPosition];
				}
				result.m_num[arrayPosition++] = addend % 10;
				addend /= 10;
			}
			while (arrayPosition < m_size) //复制没有加上的数
			{
				result.m_num[arrayPosition] = m_num[arrayPosition];
				++arrayPosition;
			}
			result.m_size = arrayPosition;
			return result;
		}
		xxxMath::BigInt BasicSelfAdd(const xxxMath::BigInt& addend) //自加
		{
			xxxMath::UINT_32 maxSize(m_size > addend.m_size ? m_size : addend.m_size);
			register xxxMath::UINT_32 arrayPosition(0), addBuffer(0);
			if (GetMaxLength() <= maxSize) //长度不够就重新分配空间大小
			{
				Reallocate(maxSize);
			}
			while (arrayPosition <= maxSize) //自加
			{
				if (arrayPosition < m_size) //加上被加数
				{
					addBuffer += m_num[arrayPosition];
				}
				if (arrayPosition < addend.m_size) //加上加数
				{
					addBuffer += addend.m_num[arrayPosition];
				}
				m_num[arrayPosition] = addBuffer % 10; //获取结果
				addBuffer /= 10; //获取进位
				++arrayPosition;
			}
			m_size = maxSize;
			if (m_num[maxSize]) //有进位
			{
				++m_size;
			}
			return *this;
		}
		xxxMath::BigInt BasicSelfAdd(xxxMath::UINT_32 addend) //自加
		{
			xxxMath::UINT_32 maxSize(m_size > 10 ? m_size : 10); //结果最大长度
			register xxxMath::UINT_32 arrayPosition(0);
			if (GetMaxLength() <= maxSize) //空间不足
			{
				Reallocate(maxSize);
			}
			while (addend)
			{
				if (arrayPosition < m_size)
				{
					addend += m_num[arrayPosition];
				}
				m_num[arrayPosition++] = addend % 10;
				addend /= 10;
			}
			if (arrayPosition > m_size)
			{
				m_size = arrayPosition;
			}
			return *this;
		}
		xxxMath::BigInt BasicSubtract(const xxxMath::BigInt& subtrahend) const //要求被减数大于等于减数
		{
			register xxxMath::UINT_32 arrayPosition(0), realPosition(0);
			register xxxMath::INT_32 subtractBuffer(0);
			xxxMath::BigInt result;
			result.Allocate(m_size);
			while (arrayPosition < m_size)
			{
				subtractBuffer += m_num[arrayPosition];
				if (arrayPosition < subtrahend.m_size)
				{
					subtractBuffer -= subtrahend.m_num[arrayPosition];
				}
				result.m_num[arrayPosition++] = (10 + subtractBuffer) % 10;
				if ((10 + subtractBuffer) % 10)
				{
					realPosition = arrayPosition;
				}
				subtractBuffer = -(subtractBuffer < 0);
			}
			result.m_size = realPosition;
			return result;
		}
		xxxMath::BigInt BasicSubtract(xxxMath::UINT_32 subtrahend) const //要求被减数大于等于减数
		{
			register xxxMath::UINT_32 arrayPosition(0), realPosition(0);
			register xxxMath::INT_32 subtractBuffer(0);
			xxxMath::BigInt result;
			result.Allocate(m_size > 10 ? m_size : 10);
			while (arrayPosition < m_size)
			{
				subtractBuffer += m_num[arrayPosition];
				if (subtrahend)
				{
					subtractBuffer -= subtrahend % 10;
					subtrahend /= 10;
				}
				result.m_num[arrayPosition++] = (10 + subtractBuffer) % 10;
				if ((10 + subtractBuffer) % 10)
				{
					realPosition = arrayPosition;
				}
				subtractBuffer = -(subtractBuffer < 0);
			}
			result.m_size = realPosition;
			return result;
		}
		xxxMath::BigInt& BasicSelfSubtract(const xxxMath::BigInt& subtrahend) //要求被减数大于等于减数
		{
			register xxxMath::UINT_32 arrayPosition(0), realPosition(0);
			register xxxMath::INT_32 subtractBuffer(0);
			while (arrayPosition < subtrahend.m_size)
			{
				subtractBuffer += m_num[arrayPosition] - subtrahend.m_num[arrayPosition];
				m_num[arrayPosition++] = (10 + subtractBuffer) % 10;
				if ((10 + subtractBuffer) % 10) //减法其他地方也应参照,避免出现-10的情况
				{
					realPosition = arrayPosition;
				}
				subtractBuffer = -(subtractBuffer < 0);
			}
			while (subtractBuffer)
			{
				subtractBuffer += m_num[arrayPosition];
				m_num[arrayPosition++] = (10 + subtractBuffer) % 10;
				if ((10 + subtractBuffer) % 10)
				{
					realPosition = arrayPosition;
				}
				subtractBuffer = -(subtractBuffer < 0);
			}
			if (arrayPosition == m_size)
			{
				m_size = realPosition;
			}
			return *this;
		}
		xxxMath::BigInt& BasicSelfSubtract(xxxMath::UINT_32 subtrahend) //未完
		{
			register xxxMath::UINT_32 arrayPosition(0), realPosition(0);
			register xxxMath::INT_32 subtractBuffer(0);
			while (arrayPosition < m_size)
			{
				subtractBuffer += m_num[arrayPosition];
				if (subtrahend)
				{
					subtractBuffer -= subtrahend % 10;
					subtrahend /= 10;
				}
				m_num[arrayPosition++] = (10 + subtractBuffer) % 10;
				if ((10 + subtractBuffer) % 10)
				{
					realPosition = arrayPosition;
				}
				subtractBuffer = -(subtractBuffer < 0);
			}
			m_size = realPosition;
			return *this;
		}
		xxxMath::BigInt& BasicSelfShiftSubtract(const xxxMath::BigInt& subtrahend, const xxxMath::UINT_32& shift) //未完
		{
			register xxxMath::UINT_32 arrayPosition(0), realPosition(0);
			register xxxMath::INT_32 subtractBuffer(0);
			while (arrayPosition < subtrahend.m_size)
			{
				subtractBuffer += m_num[arrayPosition] - subtrahend.m_num[arrayPosition];
				m_num[arrayPosition++] = (10 + subtractBuffer) % 10;
				if ((10 + subtractBuffer) % 10) //减法其他地方也应参照,避免出现-10的情况
				{
					realPosition = arrayPosition;
				}
				subtractBuffer = -(subtractBuffer < 0);
			}
			while (subtractBuffer)
			{
				subtractBuffer += m_num[arrayPosition];
				m_num[arrayPosition++] = (10 + subtractBuffer) % 10;
				if ((10 + subtractBuffer) % 10)
				{
					realPosition = arrayPosition;
				}
				subtractBuffer = -(subtractBuffer < 0);
			}
			if (arrayPosition == m_size)
			{
				m_size = realPosition;
			}
			return *this;
		}
		xxxMath::BigInt BasicMultiply_Manual(const xxxMath::BigInt& multiplier) const
		{
			xxxMath::BigInt result;
			xxxMath::UINT_32 multiplyBuffer(0), carryPosition, maxReachPosition(0); //maxReachPosition在最后可为理论最大位数位置的前一个位置
			//maxReachPosition:从最开始到某个阶段乘法所能达到的最大数位,加一为进位
			result.Allocate(m_size + multiplier.m_size); //分配空间
			result.m_num[0] = 0; //初始化第一位数字,后面的位轮到他之前就初始化
			for (xxxMath::UINT_32 position1(0); position1 < m_size; ++position1) //被乘数数位
			{
				for (xxxMath::UINT_32 position2(0); position2 < multiplier.m_size; ++position2) //乘数数位
				{
					result.m_num[maxReachPosition + 1] = 0;
					multiplyBuffer = m_num[position1] * multiplier.m_num[position2] + result.m_num[position1 + position2];
					result.m_num[position1 + position2] = multiplyBuffer % 10;
					multiplyBuffer /= 10;
					carryPosition = position1 + position2 + 1;
					while (multiplyBuffer)
					{
						if (carryPosition <= maxReachPosition)
						{
							multiplyBuffer += result.m_num[carryPosition];
						}
						result.m_num[carryPosition] = multiplyBuffer % 10;
						multiplyBuffer /= 10;
						++carryPosition;
					}
					if (--carryPosition > maxReachPosition)
					{
						maxReachPosition = carryPosition;
					}
				}
			}
			result.m_size = result.m_num[maxReachPosition] ? maxReachPosition + 1 : maxReachPosition;
			return result;
		}
		xxxMath::BigInt BasicMultiply_Manual(xxxMath::UINT_32 multiplier) const
		{
			xxxMath::BigInt result;
			xxxMath::UINT_32 multiplyBuffer(0), arrayPosition(0);
			result.Allocate(m_size + 10); //分配空间
			while (arrayPosition < m_size) //被乘数数位
			{
				multiplyBuffer += m_num[arrayPosition] * multiplier;
				result.m_num[arrayPosition] = multiplyBuffer % 10;
				multiplyBuffer /= 10;
				++arrayPosition;
			}
			while (multiplyBuffer) //被乘数数位进位
			{
				result.m_num[arrayPosition] = multiplyBuffer % 10;
				multiplyBuffer /= 10;
				++arrayPosition;
			}
			result.m_size = arrayPosition;
			return result;
		}
		xxxMath::BigInt& BasicSelfMultiply_Manual(const xxxMath::BigInt& multiplier)
		{
			return this->Move(BasicMultiply_Manual(multiplier));
		}
		xxxMath::BigInt& BasicSelfMultiply_Manual(const xxxMath::UINT_32& multiplier)
		{
			xxxMath::UINT_32 multiplyBuffer(0), arrayPosition(0);
			if (GetMaxLength() < m_size + 10)
			{
				Reallocate(m_size + 10);
			}
			while (arrayPosition < m_size) //被乘数数位
			{
				multiplyBuffer += m_num[arrayPosition] * multiplier;
				m_num[arrayPosition] = multiplyBuffer % 10;
				multiplyBuffer /= 10;
				++arrayPosition;
			}
			while (multiplyBuffer)
			{
				m_num[arrayPosition] = multiplyBuffer % 10;
				multiplyBuffer /= 10;
				++arrayPosition;
			}
			m_size = arrayPosition;
			return *this;
		}
		xxxMath::BigInt BasicMultiply(const xxxMath::BigInt& multiplier) const
		{
			return BasicMultiply_Manual(multiplier);
		}
		xxxMath::BigInt BasicMultiply(const xxxMath::INT_32& multiplier) const
		{
			return BasicMultiply_Manual(multiplier);
		}
		xxxMath::BigInt BasicSelfMultiply(const xxxMath::BigInt& multiplier)
		{
			return BasicSelfMultiply_Manual(multiplier);
		}
		xxxMath::BigInt BasicSelfMultiply(const xxxMath::INT_32& multiplier)
		{
			return BasicSelfMultiply_Manual(multiplier);
		}
		//结果符号:商:被除数除数符号的积 (10)/(4)=2 (-10)/(4)=-2 (-10)/(-4)=2 (10)/(-4)=-2,模:被除数的符号 (-10)%(-4)=-2 (10)%(-4)=2 (10)%(4)=2 (-10)%(4)=-2
		xxxMath::BigInt BasicDivideModulo_Manual(const xxxMath::BigInt& divisor, xxxMath::BigInt* quotient) const //大数整除取模运算
		{ //未来详细解释每一步-1的原因
			xxxMath::BigInt modulo, multipleDivisor;
			//modulo:储存模的结果
			//multipleDivisor:某倍的除数,作为中间值,用于后面减去某一位结果的值
			xxxMath::UINT_32 divideResult, divisorFirst64, moduloFirst64, dividePosition, isFirstZero;
			//divideResult:除法计算一个位的结果
			//divisorFirst64:除数首两位的值
			//moduloFirst64:模首两位的值
			//dividePosition::除法中结果所保存在数组中的位置,同时也是数组的长度
			//isFirstZero:标记除法结果是不是首位,避免无用零出现在数的前面
			modulo.Allocate(m_size);
			xxxMath::UINT_32* pOldModulo(modulo.m_num), * pDividend(m_num + m_size);
			//pOldModulo:储存模结果数组的指针,后面模数组指针将发生后移,在循环中作为循环结束的依据
			modulo.m_size = divisor.m_size;
			modulo.m_num += m_size; //将模的指针指向最后一个元素,后续将不断回到初始位置,并拷贝被除数相应位的数字
			divisorFirst64 = divisor.m_size > 1 ? (divisor.m_num[divisor.m_size - 1] * 10 + divisor.m_num[divisor.m_size - 2]) : divisor.m_num[divisor.m_size - 1]; //除数前两位,64是因为未来采用(UINT_MAX + 1)进制进行存储
			if (quotient) //如果需要求商
			{
				dividePosition = 0;
				isFirstZero = 1;
				if (quotient->m_num)
				{
					quotient->Free();
				}
				quotient->Allocate(m_size - divisor.m_size + 1);
			}
			for (xxxMath::INT_32 repeatCount(modulo.m_size); repeatCount > 0; --repeatCount)
			{
				--modulo.m_num;
				--pDividend;
				*modulo.m_num = *pDividend;
			}
			while (modulo.m_num >= pOldModulo)
			{
				moduloFirst64 = modulo.m_size > 1 ? (modulo.m_num[modulo.m_size - 1] * 10 + modulo.m_num[modulo.m_size - 2]) : (modulo.m_size == 1 ? modulo.m_num[0] : 0);
				if (moduloFirst64 > divisorFirst64) //modulo>divisor,divideResult>=1
				{
					divideResult = moduloFirst64 / divisorFirst64;
					multipleDivisor.Move(divisor.BasicMultiply(divideResult));
					if (multipleDivisor.BasicCompare(modulo) > 0)
					{
						--divideResult;
						multipleDivisor.BasicSelfSubtract(divisor);
					}
					modulo.BasicSelfSubtract(multipleDivisor);
				} //当模为0时是否还可以比较
				else if (modulo.BasicCompare(divisor) < 0 || modulo.m_size > divisor.m_size) //modulo < divisor
				{
					if (modulo.m_num == pOldModulo && modulo.m_size == divisor.m_size) //被除数已经全复制到模中且长度相等(前提条件:modulo<divisor,即一定不能除,函数结束)
					{
						if (quotient && dividePosition != 0)
						{
							quotient->m_num[dividePosition++] = 0;
						}
						break;
					} //(dividePosition || dividePosition == 0) 
					if (quotient && modulo.m_size == divisor.m_size) //模长等于被除数长度,即必须复制一位
					{
						if (dividePosition)
						{
							quotient->m_num[dividePosition++] = 0;
						}
						--modulo.m_num;
						--pDividend;
						if (modulo.m_size || *pDividend)
						{
							*modulo.m_num = *pDividend;
							++modulo.m_size;
						}
					}
					divideResult = (moduloFirst64 * 10 + (modulo.m_size >= 3 ? modulo.m_num[modulo.m_size - 3] : modulo.m_num[modulo.m_size - 2])) / divisorFirst64;
					multipleDivisor.Move(divisor.BasicMultiply(divideResult));
					if (multipleDivisor.BasicCompare(modulo) > 0)
					{
						--divideResult;
						multipleDivisor.BasicSelfSubtract(divisor);
					}
					modulo.BasicSelfSubtract(multipleDivisor);
				}
				else //modulo >= divisor
				{
					divideResult = 1;
					modulo.BasicSelfSubtract(divisor);
				}
				if (quotient)
				{
					isFirstZero = 0;
					quotient->m_num[dividePosition++] = divideResult;
					//printf("%d\n", divideResult);
				}
				if (modulo.m_num > pOldModulo) //模本身为0时再加一个零时长度应不变 
				{
					--modulo.m_num;
					--pDividend;
					if (modulo.m_size || *pDividend)
					{
						*modulo.m_num = *pDividend;
						++modulo.m_size;
					}
				}
				else
				{
					break;
				}
				while (modulo.m_size < divisor.m_size)
				{
					if (quotient)
					{
						quotient->m_num[dividePosition++] = 0;
					}
					if (modulo.m_num == pOldModulo)
					{
						goto Result;
					}
					--modulo.m_num;
					--pDividend;
					if (modulo.m_size || *pDividend)
					{
						*modulo.m_num = *pDividend;
						++modulo.m_size;
					}
				}
			}
		Result:
			if (quotient)
			{
				quotient->m_size = dividePosition--;
				for (xxxMath::UINT_32 swapHead(0); swapHead < dividePosition; ++swapHead, --dividePosition)
				{
					if (quotient->m_num[swapHead] ^ quotient->m_num[dividePosition])
					{
						quotient->m_num[swapHead] ^= quotient->m_num[dividePosition];
						quotient->m_num[dividePosition] ^= quotient->m_num[swapHead];
						quotient->m_num[swapHead] ^= quotient->m_num[dividePosition];
					}
				}
			}
			return modulo;
		}
		xxxMath::UINT_32 BasicDivideModulo_Manual(const xxxMath::UINT_32& divisor, xxxMath::BigInt* quotient) const
		{
			xxxMath::UINT_32 modulo(0), isFirstZero, divideResult, dividePosition;
			xxxMath::INT_32 arrayPosition(m_size - 1);
			if (quotient)
			{
				quotient->Reallocate(m_size);
				dividePosition = 0;
				isFirstZero = 1;
			}
			while (arrayPosition >= 0)
			{
				modulo = modulo * 10 + m_num[arrayPosition];
				divideResult = modulo / divisor;
				modulo %= divisor;
				if (quotient)
				{
					if (isFirstZero)
					{
						if (divideResult)
						{
							isFirstZero = 0;
							quotient->m_num[dividePosition++] = divideResult;
						}
					}
					else
					{
						quotient->m_num[dividePosition++] = divideResult;
					}
				}
			}
			if (quotient)
			{
				quotient->m_size = dividePosition--;
				for (xxxMath::UINT_32 swapHead(0); swapHead < dividePosition; ++swapHead, --dividePosition)
				{
					if (quotient->m_num[swapHead] ^ quotient->m_num[dividePosition])
					{
						quotient->m_num[swapHead] ^= quotient->m_num[dividePosition];
						quotient->m_num[dividePosition] ^= quotient->m_num[swapHead];
						quotient->m_num[swapHead] ^= quotient->m_num[dividePosition];
					}
				}
			}
			return modulo;
		}
		//关于自己除别的数优化,将结果暂存到数组末端访问不到的地方
		xxxMath::BigInt BasicDivideModulo(const xxxMath::BigInt& divisor, xxxMath::BigInt& quotient) const
		{
			return BasicDivideModulo_Manual(divisor, &quotient);
		}
		xxxMath::UINT_32 BasicDivideModulo(const xxxMath::UINT_32& divisor, xxxMath::BigInt& quotient) const
		{
			return BasicDivideModulo_Manual(divisor, &quotient);
		}
		xxxMath::BigInt BasicDivide_Manual(const xxxMath::BigInt& divisor) const
		{
			xxxMath::BigInt quotient;
			BasicDivideModulo_Manual(divisor, &quotient);
			return quotient;
		}
		xxxMath::BigInt BasicDivide_Manual(const xxxMath::UINT_32& divisor) const
		{
			xxxMath::BigInt quotient;
			BasicDivideModulo_Manual(divisor, &quotient);
			return quotient;
		}
		xxxMath::BigInt BasicModulo_Manual(const xxxMath::BigInt& divisor) const
		{
			return BasicDivideModulo_Manual(divisor, nullptr);
		}
		xxxMath::UINT_32 BasicModulo_Manual(const xxxMath::UINT_32& divisor) const
		{
			return BasicDivideModulo_Manual(divisor, nullptr);
		}
		xxxMath::BigInt BasicSelfDivide_Manual(const xxxMath::BigInt& divisor)
		{
			xxxMath::BigInt quotient;
			BasicDivideModulo_Manual(divisor, &quotient);
			return this->Move(quotient);
		}
		xxxMath::BigInt BasicSelfDivide_Manual(const xxxMath::UINT_32& divisor)
		{
			xxxMath::BigInt quotient;
			BasicDivideModulo_Manual(divisor, &quotient);
			return this->Move(quotient);
		}
		xxxMath::BigInt BasicSelfModulo_Manual(const xxxMath::BigInt& divisor)
		{
			return this->Move(BasicDivideModulo_Manual(divisor, nullptr));
		}
		xxxMath::BigInt BasicSelfModulo_Manual(const xxxMath::UINT_32& divisor)
		{
			return this->Initialize(BasicDivideModulo_Manual(divisor, nullptr));
		}
		xxxMath::BigInt BasicDivide(const xxxMath::BigInt& divisor) const
		{
			return BasicDivide_Manual(divisor);
		}
		xxxMath::BigInt BasicDivide(const xxxMath::UINT_32& divisor) const
		{
			return BasicDivide_Manual(divisor);
		}
		xxxMath::BigInt BasicModulo(const xxxMath::BigInt& divisor) const
		{
			return BasicModulo_Manual(divisor);
		}
		xxxMath::UINT_32 BasicModulo(const xxxMath::UINT_32& divisor) const
		{
			return BasicModulo_Manual(divisor);
		}
		xxxMath::BigInt BasicSelfDivide(const xxxMath::BigInt& divisor)
		{
			return BasicSelfDivide_Manual(divisor);
		}
		xxxMath::BigInt BasicSelfDivide(const xxxMath::UINT_32& divisor)
		{
			return BasicSelfDivide_Manual(divisor);
		}
		xxxMath::BigInt BasicSelfModulo(const xxxMath::BigInt& divisor)
		{
			return BasicSelfModulo_Manual(divisor);
		}
		xxxMath::BigInt BasicSelfModulo(const xxxMath::UINT_32& divisor)
		{
			return BasicSelfModulo_Manual(divisor);
		}
	public: //最终常用的接口,函数版
		xxxMath::BigInt Add(const xxxMath::BigInt& addend) const
		{
			if (m_sign ^ addend.m_sign) //异号
			{
				if (!m_sign) //被加数为零
				{
					return addend;
				}
				else if (!addend.m_sign) //加数为零
				{
					return *this;
				}
				else if (m_sign > 0) //一正一负
				{
					xxxMath::BigInt result;
					xxxMath::INT_32 compareResult;
					compareResult = BasicCompare(addend);
					if (compareResult > 0)
					{
						result.Move(BasicSubtract(addend));
						result.m_sign = 1;
					}
					else if (compareResult < 0)
					{
						result.Move(addend.BasicSubtract(*this));
						result.m_sign = -1;
					}
					return result;
				}
				else //一负一正
				{
					xxxMath::BigInt result;
					xxxMath::INT_32 compareResult;
					compareResult = BasicCompare(addend);
					if (compareResult > 0)
					{
						result.Move(BasicSubtract(addend));
						result.m_sign = -1;
					}
					else if (compareResult < 0)
					{
						result.Move(addend.BasicSubtract(*this));
						result.m_sign = 1;
					}
					return result; //二者相等时不做处理,结果的默认构造函数已将之初始化为零
				}
			}
			else //同号,同正或同负
			{
				xxxMath::BigInt result(BasicAdd(addend));
				result.m_sign = m_sign;
				return result;
			}
		}
		xxxMath::BigInt Add(const xxxMath::INT_32& addend) const
		{
			xxxMath::INT_32 addendSign(addend > 0 ? 1 : (addend < 0 ? -1 : 0));
			if (m_sign ^ addendSign) //异号
			{
				xxxMath::BigInt result;
				if (!m_sign) //被加数为零
				{
					result.Initialize(addend);
					return result; //未来使用构造函数
				}
				else if (!addend) //加数为零
				{
					return *this;
				}
				else if (m_sign > 0) //一正一负
				{
					xxxMath::INT_32 compareResult;
					xxxMath::BigInt addendBuffer;
					addendBuffer.Initialize(addend);
					compareResult = BasicCompare(addendBuffer);
					if (compareResult > 0)
					{
						result.Move(BasicSubtract(addend));
						result.m_sign = 1;
					}
					else if (compareResult < 0)
					{
						result.Move(addendBuffer.BasicSubtract(*this));
						result.m_sign = -1;
					}
					return result; //二者相等时不做处理,结果的默认构造函数已将之初始化为零
				}
				else //一负一正
				{
					xxxMath::INT_32 compareResult;
					xxxMath::BigInt addendBuffer;
					addendBuffer.Initialize(addend);
					compareResult = BasicCompare(addendBuffer);
					if (compareResult > 0)
					{
						result.Move(BasicSubtract(addend));
						result.m_sign = -1;
					}
					else if (compareResult < 0)
					{
						result.Move(addendBuffer.BasicSubtract(*this));
						result.m_sign = 1;
					}
					return result;
				}
			}
			else //同号
			{
				xxxMath::BigInt result(BasicAdd(addend));
				result.m_sign = m_sign;
				return result;
			}
		}
		xxxMath::BigInt& SelfAdd(const xxxMath::BigInt& addend)
		{
			if (m_sign ^ addend.m_sign) //异号
			{
				if (!m_sign) //被加数为零
				{
					Copy(addend);
				}
				else if (!addend.m_sign) //加数为零
				{
				}
				else if (m_sign > 0) //一正一负
				{
					xxxMath::INT_32 compareResult;
					compareResult = BasicCompare(addend);
					if (compareResult > 0)
					{
						BasicSelfSubtract(addend);
						m_sign = 1;
					}
					else if (compareResult < 0)
					{
						Move(addend.BasicSubtract(*this));
						m_sign = -1;
					}
				}
				else //一负一正
				{
					xxxMath::INT_32 compareResult;
					compareResult = BasicCompare(addend);
					if (compareResult > 0)
					{
						BasicSelfSubtract(addend);
						m_sign = -1;
					}
					else if (compareResult < 0)
					{
						Move(addend.BasicSubtract(*this));
						m_sign = 1;
					}
				}
			}
			else //同号,同正或同负
			{
				BasicSelfAdd(addend);
			}
			return *this;
		}
		xxxMath::BigInt& SelfAdd(const xxxMath::INT_32& addend)
		{
			xxxMath::INT_32 addendSign(addend > 0 ? 1 : (addend < 0 ? -1 : 0));
			if (m_sign ^ addendSign) //异号
			{
				if (!m_sign) //被加数为零
				{
					Initialize(addend);
				}
				else if (!addend) //加数为零
				{
				}
				else if (m_sign > 0) //一正一负
				{
					xxxMath::INT_32 compareResult;
					xxxMath::BigInt addendBuffer;
					addendBuffer.Initialize(addend);
					compareResult = BasicCompare(addendBuffer);
					if (compareResult > 0)
					{
						Move(BasicSubtract(addend));
						m_sign = 1;
					}
					else if (compareResult < 0)
					{
						Move(addendBuffer.BasicSubtract(*this));
						m_sign = -1;
					}
				}
				else //一负一正
				{
					xxxMath::INT_32 compareResult;
					xxxMath::BigInt addendBuffer;
					addendBuffer.Initialize(addend);
					compareResult = BasicCompare(addendBuffer);
					if (compareResult > 0)
					{
						Move(BasicSubtract(addend));
						m_sign = -1;
					}
					else if (compareResult < 0)
					{
						Move(addendBuffer.BasicSubtract(*this));
						m_sign = 1;
					}
				}
			}
			else //同号
			{
				BasicSelfAdd(addend);
			}
			return *this;
		}
		xxxMath::BigInt Subtract(const xxxMath::BigInt& subtrahend) const
		{
			xxxMath::BigInt result;
			if (m_sign ^ subtrahend.m_sign) //异号
			{
				if (!m_sign) //被减数为零
				{
					result.Initialize(subtrahend); //符号错误?
				}
				else if (!subtrahend.m_sign) //减数为零
				{
					return *this;
				}
				else //一正一负或一负一正
				{
					result.Move(BasicAdd(subtrahend));
					result.m_sign = m_sign;
				}
			}
			else //同号
			{
				xxxMath::INT_32 compareResult(BasicCompare(subtrahend));
				if (compareResult > 0)
				{
					result.Move(BasicSubtract(subtrahend));
					result.m_sign = m_sign;
				}
				else if (compareResult < 0)
				{
					result.Move(subtrahend.BasicSubtract(*this));
					result.m_sign = -m_sign;
				}
				else
				{
				}
			}
			return result;
		}
		xxxMath::BigInt Subtract(const xxxMath::INT_32& subtrahend) const
		{
			xxxMath::BigInt result;
			xxxMath::INT_32 subtrahendSign = subtrahend > 0 ? 1 : (subtrahend == 0 ? 0 : -1);
			if (m_sign ^ subtrahendSign) //异号
			{
				if (!m_sign) //被减数为零
				{
					result.Initialize(subtrahend);
					result.m_sign = -subtrahendSign;
				}
				else if (!subtrahendSign) //减数为零
				{
					return *this;
				}
				else //一正一负或一负一正
				{
					result.Move(BasicAdd(subtrahend > 0 ? subtrahend : -subtrahend));
					result.m_sign = m_sign;
				}
			}
			else //同号
			{
				xxxMath::BigInt subtrahendBuffer(subtrahend);
				xxxMath::INT_32 compareResult(BasicCompare(subtrahendBuffer));
				if (compareResult > 0)
				{
					result.Move(BasicSubtract(subtrahend));
					result.m_sign = m_sign;
				}
				else if (compareResult < 0)
				{
					result.Move(subtrahendBuffer.BasicSubtract(*this));
					result.m_sign = -m_sign;
				}
				else
				{
				}
			}
			return result;
		}
		xxxMath::BigInt& SelfSubtract(const xxxMath::BigInt& subtrahend)
		{
			if (m_sign ^ subtrahend.m_sign) //异号
			{
				if (!m_sign) //被减数为零
				{
					Copy(subtrahend);
					m_sign = -subtrahend.m_sign;
				}
				else if (!subtrahend.m_sign) //减数为零
				{
				}
				else //一正一负或一负一正
				{
					BasicSelfAdd(subtrahend);
				}
			}
			else //同号
			{
				xxxMath::INT_32 compareResult(BasicCompare(subtrahend));
				xxxMath::INT_32 oldSign(m_sign);
				if (compareResult > 0)
				{
					BasicSelfSubtract(subtrahend);
					m_sign = oldSign;
				}
				else if (compareResult < 0)
				{
					oldSign = -oldSign;
					Move(subtrahend.BasicSubtract(*this));
					m_sign = oldSign;
				}
				else
				{
					Initialize(0);
				}
			}
			return *this;
		}
		xxxMath::BigInt& SelfSubtract(const xxxMath::INT_32& subtrahend)
		{
			xxxMath::BigInt result;
			xxxMath::INT_32 subtrahendSign = subtrahend > 0 ? 1 : (subtrahend == 0 ? 0 : -1);
			if (m_sign ^ subtrahendSign) //异号
			{
				if (!m_sign) //被减数为零
				{
					Initialize(subtrahend);
					m_sign = -subtrahendSign;
				}
				else if (!subtrahendSign) //减数为零
				{
				}
				else //一正一负或一负一正
				{
					BasicSelfAdd(subtrahend > 0 ? subtrahend : -subtrahend);
				}
			}
			else //同号
			{
				xxxMath::BigInt subtrahendBuffer(subtrahend);
				xxxMath::INT_32 compareResult(BasicCompare(subtrahendBuffer));
				if (compareResult > 0)
				{
					BasicSelfSubtract(subtrahend);
				}
				else if (compareResult < 0)
				{
					Move(subtrahendBuffer.BasicSubtract(*this));
				}
				else
				{
					Initialize(0);
				}
			}
			return *this;
		}
		xxxMath::BigInt Multiply(const xxxMath::BigInt& multiplier) const
		{
			if (m_sign * multiplier.m_sign == 0)
			{
				return xxxMath::BigInt();
			}
			xxxMath::BigInt result(BasicMultiply(multiplier));
			result.m_sign = m_sign * multiplier.m_sign;
			return result;
		}
		xxxMath::BigInt Multiply(const xxxMath::INT_32& multiplier) const
		{
			xxxMath::INT_32 multiplierSign(multiplier > 0 ? 1 : (multiplier == 0 ? 0 : -1));
			if (m_sign * multiplierSign == 0)
			{
				return xxxMath::BigInt();
			}
			xxxMath::BigInt result(BasicMultiply(multiplier > 0 ? multiplier : -multiplier));
			result.m_sign = m_sign * multiplierSign;
			return result;
		}
		xxxMath::BigInt& SelfMultiply(const xxxMath::BigInt& multiplier) //自乘
		{
			if (m_sign * multiplier.m_sign == 0)
			{
				Move(xxxMath::BigInt());
				return *this;
			}
			BasicSelfMultiply(multiplier);
			m_sign *= multiplier.m_sign;
			return *this;
		}
		xxxMath::BigInt& SelfMultiply(const xxxMath::INT_32& multiplier) //自乘
		{
			xxxMath::INT_32 multiplierSign(multiplier > 0 ? 1 : (multiplier == 0 ? 0 : -1));
			if (m_sign * multiplierSign == 0)
			{
				Move(xxxMath::BigInt());
				return *this;
			}
			BasicSelfMultiply(multiplier);
			m_sign *= multiplierSign;
			return *this;
		}
		xxxMath::BigInt DivideModulo(const xxxMath::BigInt& divisor, xxxMath::BigInt& quotient) const //整除模结合
		{
			if (!divisor.m_sign) //被除数不得为零
			{
				throw("RuntimeError:Zero cannot be a divisor."); //抛出异常:运行时错误:被除数不能为零
			}
			xxxMath::BigInt modulo(BasicDivideModulo(divisor, quotient));
			modulo.m_sign = m_sign;
			quotient.m_sign = m_sign * divisor.m_sign;
			return modulo;
		}
		xxxMath::UINT_32 DivideModulo(const xxxMath::UINT_32& divisor, xxxMath::BigInt& quotient) const //整除模结合
		{
			if (!divisor) //被除数不得为零
			{
				throw("RuntimeError:Zero cannot be a divisor."); //抛出异常:运行时错误:被除数不能为零
			}
			xxxMath::UINT_32 modulo(BasicDivideModulo(divisor, quotient));
			modulo *= m_sign;
			quotient.m_sign = m_sign * (divisor > 0 ? 1 : (divisor == 0 ? 0 : -1));
			return modulo;
		}
		xxxMath::BigInt Divide(const xxxMath::BigInt& divisor) const //整除
		{
			if (!divisor.m_sign) //被除数不得为零
			{
				throw("RuntimeError:Zero cannot be a divisor."); //抛出异常:运行时错误:被除数不能为零
			}
			xxxMath::BigInt quotient(BasicDivide(divisor));
			quotient.m_sign = m_sign * divisor.m_sign;
			return quotient;
		}
		xxxMath::BigInt Divide(const xxxMath::INT_32& divisor) const //整除
		{
			if (!divisor) //被除数不得为零
			{
				throw("RuntimeError:Zero cannot be a divisor."); //抛出异常:运行时错误:被除数不能为零
			}
			xxxMath::BigInt quotient(BasicDivide(divisor > 0 ? divisor : -divisor));
			quotient.m_sign = m_sign * (divisor > 0 ? 1 : (divisor == 0 ? 0 : -1));
			return quotient;
		}
		xxxMath::BigInt& SelfDivide(const xxxMath::BigInt& divisor) //自整除
		{
			if (!divisor.m_sign) //被除数不得为零
			{
				throw("RuntimeError:Zero cannot be a divisor."); //抛出异常:运行时错误:被除数不能为零
			}
			BasicSelfDivide(divisor);
			m_sign *= divisor.m_sign;
			return *this;
		}
		xxxMath::BigInt& SelfDivide(const xxxMath::INT_32& divisor) //自整除
		{
			if (!divisor) //被除数不得为零
			{
				throw("RuntimeError:Zero cannot be a divisor."); //抛出异常:运行时错误:被除数不能为零
			}
			BasicSelfDivide(divisor > 0 ? divisor : -divisor);
			m_sign *= divisor > 0 ? 1 : (divisor == 0 ? 0 : -1);
			return *this;
		}
		xxxMath::BigInt Modulo(const xxxMath::BigInt& divisor) const //模
		{
			if (!divisor.m_sign) //被除数不得为零
			{
				throw("RuntimeError:Zero cannot be a divisor."); //抛出异常:运行时错误:被除数不能为零
			}
			xxxMath::BigInt modulo(BasicModulo(divisor));
			modulo.m_sign = m_sign;
			return modulo;
		}
		xxxMath::BigInt Modulo(const xxxMath::INT_32& divisor) const //模
		{
			if (!divisor) //被除数不得为零
			{
				throw("RuntimeError:Zero cannot be a divisor."); //抛出异常:运行时错误:被除数不能为零
			}
			xxxMath::BigInt modulo(BasicModulo(divisor > 0 ? divisor : -divisor));
			modulo.m_sign = m_sign;
			return modulo;
		}
		xxxMath::BigInt SelfModulo(const xxxMath::BigInt& divisor) //自模
		{
			if (!divisor.m_sign) //被除数不得为零
			{
				throw("RuntimeError:Zero cannot be a divisor."); //抛出异常:运行时错误:被除数不能为零
			}
			return BasicSelfModulo(divisor);
		}
		xxxMath::BigInt SelfModulo(const xxxMath::INT_32& divisor) //自模
		{
			if (!divisor) //被除数不得为零
			{
				throw("RuntimeError:Zero cannot be a divisor."); //抛出异常:运行时错误:被除数不能为零
			}
			return BasicSelfModulo(divisor > 0 ? divisor : -divisor);
		}
	public: //最终常用的接口,运算符重载版
		template <typename ValueType>
		inline xxxMath::INT_32 operator > (const ValueType& input) //大于重载
		{
			return Compare(input) > 0;
		}
		template <typename ValueType>
		inline xxxMath::INT_32 operator < (const ValueType& input) //小于重载
		{
			return Compare(input) < 0;
		}
		template <typename ValueType>
		inline xxxMath::INT_32 operator >= (const ValueType& input) //大于等于重载
		{
			return Compare(input) >= 0;
		}
		template <typename ValueType>
		inline xxxMath::INT_32 operator <= (const ValueType& input) //小于等于重载
		{
			return Compare(input) <= 0;
		}
		template <typename ValueType>
		inline xxxMath::INT_32 operator == (const ValueType& input) //等于等于重载
		{
			return Compare(input) == 0;
		}
		//xxxMath::BigInt operator = (const xxxMath::BigInt& input) //赋值重载,拷贝赋值,该不该去掉?
		//{
		//	return Initialize(input);
		//}
		inline xxxMath::BigInt operator = (xxxMath::BigInt&& input) noexcept //赋值重载,移动赋值
		{
			return Move(input);
		}
		inline xxxMath::BigInt operator = (const xxxMath::BigInt& input) //赋值重载,拷贝赋值
		{
			return Copy(input);
		}
		template <typename ValueType>
		inline xxxMath::BigInt operator = (const ValueType& input) //赋值重载,通用赋值
		{
			return Initialize(input);
		}
		template <typename ValueType>
		inline xxxMath::BigInt operator + (const ValueType& addend) //加法运算重载
		{
			return Add(addend);
		}
		template <typename ValueType>
		inline xxxMath::BigInt& operator += (const ValueType& addend) //自加运算重载
		{
			return SelfAdd(addend);
		}
		template <typename ValueType>
		inline xxxMath::BigInt operator - (const ValueType& subtrahend) //减法运算重载
		{
			return Subtract(subtrahend);
		}
		template <typename ValueType>
		inline xxxMath::BigInt operator -= (const ValueType& subtrahend) //自减运算重载
		{
			return SelfSubtract(subtrahend);
		}
		template <typename ValueType>
		inline xxxMath::BigInt operator * (const ValueType& multiplier) //乘法运算重载
		{
			return Multiply(multiplier);
		}
		template <typename ValueType>
		inline xxxMath::BigInt operator *= (const ValueType& multiplier) //自乘运算重载
		{
			return SelfMultiply(multiplier);
		}
		template <typename ValueType>
		inline xxxMath::BigInt operator / (const ValueType& divisor) //整除运算重载
		{
			return Divide(divisor);
		}
		template <typename ValueType>
		inline xxxMath::BigInt& operator /= (const ValueType& divisor) //自整除运算重载
		{
			return SelfDivide(divisor);
		}
		template <typename ValueType>
		inline xxxMath::BigInt operator % (const ValueType& divisor) //模运算重载
		{
			return Modulo(divisor);
		}
		template <typename ValueType>
		inline xxxMath::BigInt& operator %= (const ValueType& divisor) //自模运算重载
		{
			return SelfModulo(divisor);
		}
		bool IsNullptr()
		{
			return m_num == nullptr;
		}
		*/
	};
};

int main()
{
	//除法错误与未知bug, 将4个空格替代掉
	xxxMath::BigInt a(214748364), b, c, d;
	b.Copy(xxxMath::BigInt(100));
	b.OutputConsole();
	/*a = ca;
	b = cb;
	if ((!(a == 0)) && (a >= b))
	{
		c = a / b;
	} //9999/13  10/2  4000/99 9999999/4684 119/11

//1231312318457577687897987642324567864324567876543245671425346756786867867867
//1231312318767141738178325678412414124141425346756786867867867

	//98749648/154
	c.OutputConsole();
	//printf("%d\n", a.BasicShiftCompare(b,1));
	putchar('\n');
	d = a - c * b;
	d.OutputConsole();
	//构造函数加入m_num=nullptr 自加自减注意是否可以自己加或减自己
	/*for (int i(10000); i < 1000000; ++i)
	{
		printf("i:%d:\n", i);
		for (int j(1); j < i; ++j)
		{
			a = i;
			b = j;
			if ((!(a==0)) && (a >= b))
			{
				c = a / b;
			}
			if (!(c == (i / j)))
			{
				printf("error:%d//%d", i, j);
				system("pause");
			}// 10001/1
		}
		printf("end\n");
	}*/
	system("pause");
	return 0;
}
