#pragma once

#include <string>
#include <vector>

using Buffer = std::vector<unsigned char>;

class Serializer
{
public:
	// The lifetime of an object is the same as for a buffer
	Serializer(Buffer& buffer) : buffer_(buffer) {}

	void Serialize(int n)
	{
		buffer_.insert(buffer_.end(), (const unsigned char*)&n, (const unsigned char*)&n + sizeof(int));
	}
	void Serialize(const std::string& str)
	{
		Serialize((int)str.size());
		buffer_.insert(buffer_.end(), str.data(), str.data() + str.size());
	}

private:
	Buffer& buffer_;
};

class Deserializer
{
public:
	// The lifetime of an object is the same as for a buffer
	Deserializer(const Buffer& buffer) : buffer_(buffer.data()), buffer_current_pos_(buffer.data()), buffer_size_(buffer.size()) {}
	Deserializer(const unsigned char* buffer, size_t size) : buffer_(buffer), buffer_current_pos_(buffer), buffer_size_(size) {}
	const unsigned char* GetRemainData(size_t& remain_size) const { remain_size = GetRemainSize(); return buffer_current_pos_; }

	bool Deserialize(int& n)
	{
		if (GetRemainSize() < sizeof(int)) { return false; }
		n = *reinterpret_cast<const int*>(buffer_current_pos_);
		buffer_current_pos_ += sizeof(int);
		return true;
	}
	bool Deserialize(std::string& str)
	{
		int str_size = 0;
		if (!Deserialize(str_size) ||
			GetRemainSize() < str_size) {
			return false;
		}
		str.assign(buffer_current_pos_, buffer_current_pos_ + str_size);
		buffer_current_pos_ += str_size;
		return true;
	}

private:
	size_t GetRemainSize() const { return buffer_size_ - (buffer_current_pos_ - buffer_); }

	const unsigned char* buffer_;
	const unsigned char* buffer_current_pos_;
	size_t buffer_size_;
};