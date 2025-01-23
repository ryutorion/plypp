#ifndef PLYPP_H_INCLUDED
#define PLYPP_H_INCLUDED

#include <bit>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace plypp
{
	enum class PLYValueFormat
	{
		ASCII,
		BinaryLittleEndian,
		BinaryBigEndian
	};

	class PLYPolygon;
	std::optional<PLYPolygon> load(const std::filesystem::path& filePath);

	template <class T>
	concept PLYValueType = std::disjunction_v
	<
		std::is_same<T, int8_t>,
		std::is_same<T, uint8_t>,
		std::is_same<T, int16_t>,
		std::is_same<T, uint16_t>,
		std::is_same<T, int32_t>,
		std::is_same<T, uint32_t>,
		std::is_same<T, float>,
		std::is_same<T, double>
	>;

	template <PLYValueType T>
	class PLYValueProperty
	{
	public:
		PLYValueProperty() = default;

		PLYValueProperty(size_t size)
		{
			mValues.reserve(size);
		}

		T& operator[](size_t index)
		{
			return mValues[index];
		}

		const T operator[](size_t index) const
		{
			return mValues[index];
		}

	private:

		void load(std::istream& in, PLYValueFormat valueFormat)
		{
			switch(valueFormat)
			{
			case PLYValueFormat::ASCII:
				{
					T value;
					in >> value;
					mValues.emplace_back(value);
				}
				break;
			case PLYValueFormat::BinaryLittleEndian:
				{
					T value;
					in.read(reinterpret_cast<char*>(&value), sizeof(T));
					if constexpr(std::endian::native != std::endian::little)
					{
						std::reverse(
							reinterpret_cast<char*>(&value),
							reinterpret_cast<char*>(&value) + sizeof(T)
						);
					}
					mValues.emplace_back(value);
				}
				break;
			case PLYValueFormat::BinaryBigEndian:
				{
					T value;
					in.read(reinterpret_cast<char*>(&value), sizeof(T));
					if constexpr(std::endian::native != std::endian::big)
					{
						std::reverse(
							reinterpret_cast<char*>(&value),
							reinterpret_cast<char*>(&value) + sizeof(T)
						);
					}
					mValues.emplace_back(value);
				}
			}
		}

		friend std::optional<PLYPolygon> load(const std::filesystem::path& filePath);

	private:
		std::vector<T> mValues;
	};

	template <PLYValueType T>
	class PLYListProperty
	{
	public:
		PLYListProperty() = default;

		PLYListProperty(size_t size, size_t listSizeByteSize)
			: mListSizeByteSize(listSizeByteSize)
		{
			mValues.reserve(size);
		}

		std::vector<T>& operator[](size_t index)
		{
			return mValues[index];
		}

		const std::vector<T>& operator[](size_t index) const
		{
			return mValues[index];
		}

		size_t totalSize() const
		{
			return mTotalSize;
		}

		size_t minSize() const
		{
			return mMinSize;
		}

		size_t maxSize() const
		{
			return mMaxSize;
		}
	private:

		void load(std::istream& in, PLYValueFormat valueFormat)
		{
			bool needSwap = false;
			switch(valueFormat)
			{
			case PLYValueFormat::BinaryLittleEndian:
				needSwap = std::endian::native != std::endian::little;
				break;
			case PLYValueFormat::BinaryBigEndian:
				needSwap = std::endian::native != std::endian::big;
				break;
			}

			size_t size;
			std::vector<T> values;
			switch(valueFormat)
			{
			case PLYValueFormat::ASCII:
				in >> size;
				values.reserve(size);
				for(size_t i = 0; i < size; ++i)
				{
					T value;
					in >> value;
					values.emplace_back(value);
				}
				break;
			case PLYValueFormat::BinaryLittleEndian:
			case PLYValueFormat::BinaryBigEndian:
				switch(mListSizeByteSize)
				{
				case 1:
					{
						uint8_t v;
						in.read(reinterpret_cast<char*>(&v), sizeof(v));
						size = v;
					}
					break;
				case 2:
					{
						uint16_t v;
						in.read(reinterpret_cast<char*>(&v), sizeof(v));
						if(needSwap)
						{
							std::reverse(
								reinterpret_cast<char*>(&v),
								reinterpret_cast<char*>(&v) + sizeof(v)
							);
						}
						size = v;
					}
					break;
				case 4:
					{
						uint32_t v;
						in.read(reinterpret_cast<char*>(&v), sizeof(v));
						if(needSwap)
						{
							std::reverse(
								reinterpret_cast<char*>(&v),
								reinterpret_cast<char*>(&v) + sizeof(v)
							);
						}
						size = v;
					}
					break;
				}
				values.reserve(size);
				for(size_t i = 0; i < size; ++i)
				{
					T value;
					in.read(reinterpret_cast<char*>(&value), sizeof(T));
					if constexpr(std::endian::native != std::endian::little)
					{
						std::reverse(
							reinterpret_cast<char*>(&value),
							reinterpret_cast<char*>(&value) + sizeof(T)
						);
					}
					values.emplace_back(value);
				}
				break;
				in.read(reinterpret_cast<char*>(&size), sizeof(size_t));
				if constexpr(std::endian::native != std::endian::big)
				{
					std::reverse(
						reinterpret_cast<char*>(&size),
						reinterpret_cast<char*>(&size) + sizeof(size_t)
					);
				}

				values.reserve(size);
				for(size_t i = 0; i < size; ++i)
				{
					T value;
					in.read(reinterpret_cast<char*>(&value), sizeof(value));
					if(needSwap)
					{
						std::reverse(
							reinterpret_cast<char*>(&value),
							reinterpret_cast<char*>(&value) + sizeof(T)
						);
					}
					values.emplace_back(value);
				}
				break;
			};

			mValues.emplace_back(std::move(values));

			mTotalSize += size;
			if(mMinSize == 0 || size < mMinSize)
			{
				mMinSize = size;
			}

			if(size > mMaxSize)
			{
				mMaxSize = size;
			}
		}

		friend std::optional<PLYPolygon> load(const std::filesystem::path& filePath);

	private:
		std::vector<std::vector<T>> mValues;
		size_t mListSizeByteSize;
		size_t mTotalSize = 0;
		size_t mMinSize = 0;
		size_t mMaxSize = 0;
	};

	using PLYProperty = std::variant
	<
		PLYValueProperty<int8_t>,
		PLYValueProperty<uint8_t>,
		PLYValueProperty<int16_t>,
		PLYValueProperty<uint16_t>,
		PLYValueProperty<int32_t>,
		PLYValueProperty<uint32_t>,
		PLYValueProperty<float>,
		PLYValueProperty<double>,
		PLYListProperty<int8_t>,
		PLYListProperty<uint8_t>,
		PLYListProperty<int16_t>,
		PLYListProperty<uint16_t>,
		PLYListProperty<int32_t>,
		PLYListProperty<uint32_t>,
		PLYListProperty<float>,
		PLYListProperty<double>
	>;

	class PLYElement
	{
	public:
		PLYElement() = default;

		PLYElement(size_t size)
			: mSize(size)
		{
		}

		template <PLYValueType T>
		const auto& getValueProperty(const std::string& name) const
		{
			return std::get<PLYValueProperty<T>>(getProperty(name));
		}

		template <PLYValueType T>
		const auto& getListProperty(const std::string& name) const
		{
			return std::get<PLYListProperty<T>>(getProperty(name));
		}

		size_t size() const
		{
			return mSize;
		}

		template <PLYValueType T>
		bool containsValueProperty(const std::string& name) const
		{
			return
				mPropertyIndices.contains(name) &&
				std::holds_alternative<PLYValueProperty<T>>(getProperty(name));
		}

		template <PLYValueType T>
		bool containsListProperty(const std::string& name) const
		{
			return
				mPropertyIndices.contains(name) &&
				std::holds_alternative<PLYListProperty<T>>(getProperty(name));
		}

	private:
		friend std::optional<PLYPolygon> load(const std::filesystem::path& filePath);

		void addValueProperty(const std::string& name, const std::string& typeName)
		{
			if(typeName == "int8" || typeName == "char")
			{
				mProperties.emplace_back(PLYValueProperty<int8_t>(mSize));
			}
			else if(typeName == "uint8" || typeName == "uchar")
			{
				mProperties.emplace_back(PLYValueProperty<uint8_t>(mSize));
			}
			else if(typeName == "int16" || typeName == "short")
			{
				mProperties.emplace_back(PLYValueProperty<int16_t>(mSize));
			}
			else if(typeName == "uint16" || typeName == "ushort")
			{
				mProperties.emplace_back(PLYValueProperty<uint16_t>(mSize));
			}
			else if(typeName == "int32" || typeName == "int")
			{
				mProperties.emplace_back(PLYValueProperty<int32_t>(mSize));
			}
			else if(typeName == "uint32" || typeName == "uint")
			{
				mProperties.emplace_back(PLYValueProperty<uint32_t>(mSize));
			}
			else if(typeName == "float32" || typeName == "float")
			{
				mProperties.emplace_back(PLYValueProperty<float>(mSize));
			}
			else if(typeName == "float64" || typeName == "double")
			{
				mProperties.emplace_back(PLYValueProperty<double>(mSize));
			}

			mPropertyIndices[name] = mProperties.size() - 1;
		}

		void addListProperty(const std::string& name, const std::string& typeName, size_t listSizeByteSize)
		{
			if(typeName == "int8" || typeName == "char")
			{
				mProperties.emplace_back(PLYListProperty<int8_t>(mSize, listSizeByteSize));
			}
			else if(typeName == "uint8" || typeName == "uchar")
			{
				mProperties.emplace_back(PLYListProperty<uint8_t>(mSize, listSizeByteSize));
			}
			else if(typeName == "int16" || typeName == "short")
			{
				mProperties.emplace_back(PLYListProperty<int16_t>(mSize, listSizeByteSize));
			}
			else if(typeName == "uint16" || typeName == "ushort")
			{
				mProperties.emplace_back(PLYListProperty<uint16_t>(mSize, listSizeByteSize));
			}
			else if(typeName == "int32" || typeName == "int")
			{
				mProperties.emplace_back(PLYListProperty<int32_t>(mSize, listSizeByteSize));
			}
			else if(typeName == "uint32" || typeName == "uint")
			{
				mProperties.emplace_back(PLYListProperty<uint32_t>(mSize, listSizeByteSize));
			}
			else if(typeName == "float32" || typeName == "float")
			{
				mProperties.emplace_back(PLYListProperty<float>(mSize, listSizeByteSize));
			}
			else if(typeName == "float64" || typeName == "double")
			{
				mProperties.emplace_back(PLYListProperty<double>(mSize, listSizeByteSize));
			}

			mPropertyIndices[name] = mProperties.size() - 1;
		}

		PLYProperty& getProperty(const std::string& name)
		{
			return mProperties[mPropertyIndices.at(name)];
		}

		const PLYProperty& getProperty(const std::string& name) const
		{
			return mProperties[mPropertyIndices.at(name)];
		}

	private:
		size_t mSize = 0;
		std::vector<PLYProperty> mProperties;
		std::unordered_map<std::string, size_t> mPropertyIndices;
	};

	class PLYPolygon
	{
	public:
		PLYElement& getElement(const std::string& name)
		{
			return mElements[name];
		}

		const PLYElement& getElement(const std::string& name) const
		{
			return mElements.at(name);
		}

		bool containsElement(const std::string& name) const
		{
			return mElements.contains(name);
		}

	private:
		friend std::optional<PLYPolygon> load(const std::filesystem::path& filePath);

		void addElement(const std::string& name, size_t size)
		{
			mElements.emplace(name, size);
		}

	private:
		std::unordered_map<std::string, PLYElement> mElements;
	};

	std::optional<PLYPolygon> load(const std::filesystem::path& filePath)
	{
		auto split = [](const std::string& str) -> std::vector<std::string>
			{
				std::vector<std::string> tokens;
				std::string token;
				std::istringstream in(str);

				while(in >> token)
				{
					tokens.push_back(token);
				}

				return tokens;
			};

		if(!std::filesystem::exists(filePath))
		{
			std::cerr << "The file " << filePath << " does not exist." << std::endl;
			return std::nullopt;
		}

		std::ifstream fin(filePath, std::ios::binary);
		if(!fin)
		{
			std::cerr << "Failed to open the file " << filePath << "." << std::endl;
			return std::nullopt;
		}

		std::string line;
		if(!std::getline(fin, line) || line != "ply")
		{
			std::cerr << "The file is not a PLY file." << std::endl;
			return std::nullopt;
		}

		std::vector<std::string> elementNames;
		std::unordered_map<std::string, std::vector<std::string>> propertyNamesPerElement;

		PLYPolygon polygon;
		PLYValueFormat valueFormat = PLYValueFormat::ASCII;

		while(std::getline(fin, line))
		{
			if(line.starts_with("comment"))
			{
				continue;
			}
			else if(line == "end_header")
			{
				break;
			}

			auto tokens = split(line);

			if(tokens[0] == "property")
			{
				auto& elementName = elementNames.back();
				auto& element = polygon.getElement(elementName);
				auto& propertyNames = propertyNamesPerElement[elementName];

				if(tokens[1] == "list")
				{
					size_t listSizeByteSize;
					if(tokens[2] == "uint8" || tokens[2] == "uchar")
					{
						listSizeByteSize = 1;
					}
					else if(tokens[2] == "uint16" || tokens[2] == "ushort")
					{
						listSizeByteSize = 2;
					}
					else if(tokens[2] == "uint32" || tokens[2] == "uint")
					{
						listSizeByteSize = 4;
					}
					else
					{
						std::cerr << "Unsupported list size type." << std::endl;
						return std::nullopt;
					}

					propertyNames.emplace_back(tokens[4]);
					element.addListProperty(propertyNames.back(), tokens[3], listSizeByteSize);
				}
				else
				{
					propertyNames.emplace_back(tokens[2]);
					element.addValueProperty(propertyNames.back(), tokens[1]);
				}
			}
			else if(tokens[0] == "element")
			{
				elementNames.emplace_back(tokens[1]);

				size_t size;
				std::istringstream(tokens[2]) >> size;

				polygon.addElement(elementNames.back(), size);
			}
			else if(tokens[0] == "format")
			{
				if(tokens[1] == "ascii")
				{
					valueFormat = PLYValueFormat::ASCII;
				}
				else if(tokens[1] == "binary_little_endian")
				{
					valueFormat = PLYValueFormat::BinaryLittleEndian;
				}
				else if(tokens[1] == "binary_big_endian")
				{
					valueFormat = PLYValueFormat::BinaryBigEndian;
				}
			}
		}

		for(const std::string& elementName : elementNames)
		{
			auto& element = polygon.getElement(elementName);
			auto& propertyNames = propertyNamesPerElement[elementName];

			switch(valueFormat)
			{
			case PLYValueFormat::ASCII:
				for(size_t i = 0; i < element.size(); ++i)
				{
					if(!std::getline(fin, line))
					{
						std::cerr << "Failed to read the property values." << std::endl;
						return std::nullopt;
					}

					std::istringstream in(line);

					for(const std::string& propertyName : propertyNames)
					{
						std::visit([&in, valueFormat](auto& property)
							{
								property.load(in, valueFormat);
							},
							element.getProperty(propertyName)
						);
					}
				}
				break;
			default:
				for(size_t i = 0; i < element.size(); ++i)
				{
					for(const std::string& propertyName : propertyNames)
					{
						std::visit([&fin, valueFormat](auto& property)
							{
								property.load(fin, valueFormat);
							},
							element.getProperty(propertyName)
						);
					}
				}
				break;
			}
		}

		return polygon;
	}
}

#endif // PLYPP_H_INCLUDED