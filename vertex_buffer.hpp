//
// Created by Saliya Ekanayake on 7/9/17.
//

#ifndef CLIONCPP_VERTEX_BUFFER_HPP
#define CLIONCPP_VERTEX_BUFFER_HPP

#endif //CLIONCPP_VERTEX_BUFFER_HPP

class vertex_buffer{
public:
  vertex_buffer(){};
  vertex_buffer(int offset_factor, std::shared_ptr<short> buffer){
    this->offset_factor = offset_factor;
    this->buffer = buffer;
  }
  virtual ~vertex_buffer(){}

  int get_offset_factor() const {
    return offset_factor;
  }

  void set_offset_factor(int offset_factor) {
    this->offset_factor = offset_factor;
  }

  void set_buffer(std::shared_ptr<short> buffer) {
    this->buffer = buffer;
  }

  std::shared_ptr<short> get_buffer(){
    return buffer;
  }

private:
  int offset_factor;

protected:
  std::shared_ptr<short> buffer;
};

