/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014,  Regents of the University of California.
 *
 * This file is part of NDN repo-ng (Next generation of NDN repository).
 * See AUTHORS.md for complete list of repo-ng authors and contributors.
 *
 * repo-ng is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * repo-ng is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * repo-ng, e.g., in COPYING.md file.  if (not, see <http://www.gnu.org/licenses/>.
 */

/**
 * This file demonstrates how to generate data to be stored in repo with repo watch
 * protocol and repo insert protocol.
 * The details of the protocols can be find here
 *  <http://redmine.named-data.net/projects/repo-ng/wiki/Watched_Prefix_Insertion_Protocol>
 *  <http://redmine.named-data.net/projects/repo-ng/wiki/Basic_Repo_Insertion_Protocol>
 *
 * This file is used for debugging purpose. There are two modes for users to assign
 * names for the data.
 * 1)read the data name from a specific file
 * 2)input a prefix and a random version number will be added automatically
 * Users need to run nfd and repo-ng and set up specific repo protocols mentioned
 * above before running this program.
 * The description of command parameter can be found in the function usage().
 */

#include "common.hpp"
#include <fstream>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <string>
#include <boost/random.hpp>
#include <iostream>

namespace repo {

using namespace ndn::time;

static const milliseconds DEFAULT_TIME_INTERVAL(2000);

enum Mode
{
  AUTO,
  READFILE
};

class Publisher
{
public:
  class Error : public std::runtime_error
  {
  public:
    explicit
    Error(const std::string& what)
      : std::runtime_error(what)
    {
    }
  };

public:
  Publisher()
    : mode(AUTO)
    , dataPrefix(Name("/example/data/1"))
    , timeInterval(DEFAULT_TIME_INTERVAL)
    , duration(0)
    , m_scheduler(m_face.getIoService())
    , m_randomGenerator(static_cast<unsigned int> (0))
    , m_range(m_randomGenerator, boost::uniform_int<> (0,400000))
    , number(0)
  {
  }

  void
  run();

  void
  autoGenerate();

  void
  generateFromFile();

  void
  normalGenerate();

  ndn::shared_ptr<ndn::Data>
  createData(const ndn::Name& name);

  void
  onRegisterSuccess(const ndn::Name& prefix);

  void
  onRegisterFailed(const ndn::Name& prefix, const std::string& reason);
public:
  std::ifstream insertStream;
  Mode mode;
  Name dataPrefix;
  milliseconds timeInterval;
  milliseconds duration;
  uint64_t number;

private:
  ndn::Face m_face;
  ndn::Scheduler m_scheduler;
  boost::mt19937 m_randomGenerator;
  boost::variate_generator<boost::mt19937&, boost::uniform_int<> > m_range;
};

void
Publisher::run()
{
  m_face.setInterestFilter(dataPrefix,
                           bind(&Publisher::onInterest, this, _1, _2),
                           bind(&Publisher::onRegisterSuccess, this, _1),
                           bind(&Publisher::onRegisterFailed, this, _1, _2));
  
 
  m_face.processEvents(duration);
}


void
Publisher::onRegisterSuccess(const Name& prefix)
{
   if (number != 0)
  {
    normalGenerate();
    return;
  }
  if (mode == AUTO) {
    m_scheduler.scheduleEvent(timeInterval,
                            ndn::bind(&Publisher::autoGenerate, this));
  }
  else {
    m_scheduler.scheduleEvent(timeInterval,
                              ndn::bind(&Publisher::generateFromFile, this));
  }
}

void
Publisher::onRegisterFailed(const ndn::Name& prefix, const std::string& reason)
{
  throw Error("onRegisterFailed: " + reason);
}

void
Publisher::autoGenerate()
{
  Name name = dataPrefix;
  name.appendNumber(m_range());
  ndn::shared_ptr<Data> data = createData(name);
  // std::cout<<"data name = "<<data->getName()<<std::endl;
  m_face.put(*data);
  m_scheduler.scheduleEvent(timeInterval,
                            ndn::bind(&Publisher::autoGenerate, this));
}

void
Publisher::normalGenerate()
{
  for (uint64_t i = 0; i < number; i++)
  {
    Name name = dataPrefix;
    name.appendNumber(i);
    std::cout<<"data name "<<name<<std::endl;
    ndn::shared_ptr<Data> data = createData(name);
    m_face.put(*data);
  }
  while (1);
}


void
Publisher::generateFromFile()
{
  if (insertStream.eof()) {
    m_face.getIoService().stop();
    return;
   }
  std::string name;
  getline(insertStream, name);
  ndn::shared_ptr<Data> data = createData(Name(name));
  m_face.put(*data);
  m_scheduler.scheduleEvent(timeInterval,
                            ndn::bind(&Publisher::generateFromFile, this));
}

ndn::shared_ptr<Data>
Publisher::createData(const Name& name)
{
  static ndn::KeyChain keyChain;
  static std::vector<uint8_t> content(15, '-');

  ndn::shared_ptr<ndn::Data> data = ndn::make_shared<Data>();
  data->setName(name);
  data->setContent(&content[0], content.size());
  keyChain.sign(*data);
  return data;
}

static void
usage()
{
  std::cerr
      << " Publisher [-d dataPrefix] [-f filename] [-s duration time] [-t generate time interval] \n"
      << "  -d: specify the data prefix publisher generate\n"
      << "  -f: specify filename that publish would read from\n"
      << "  -s: specify the time duration of generate data\n"
      << "  -t: specify the time interval between two data generated\n"
      << "  -n: specify the number of data to be generated\n"
      << std::endl;
  exit(1);
}

int
main(int argc, char** argv)
{
  Publisher generator;
  bool isAuto = false;
  bool isRead = false;
  int opt;
  while ((opt = getopt(argc, argv, "d:f:s:t:n:")) != -1) {
    switch (opt) {
    case 'd':
      {
        generator.dataPrefix = Name(std::string(optarg));
        generator.mode = AUTO;
        isAuto = true;
      }
      break;
    case 'f':
      {
        isRead = true;
        generator.mode = READFILE;
        std::string str = std::string(optarg);
        generator.insertStream.open(str.c_str());
         if (!generator.insertStream.is_open()) {
          std::cerr << "ERROR: cannot open " << std::string(optarg) << std::endl;
          return 1;
        }
      }
      break;
    case 's':
      try {
        generator.duration = milliseconds(boost::lexical_cast<uint64_t>(optarg));
      }
      catch (boost::bad_lexical_cast&) {
        std::cerr << "-s option should be an integer.";
        return 1;
      }
      break;
    case 't':
      try {
        generator.timeInterval = milliseconds(boost::lexical_cast<uint64_t>(optarg));
      }
      catch (boost::bad_lexical_cast&) {
        std::cerr << "-t option should be an integer.";
        return 1;
      }
      break;
    case 'n':
      try {
        generator.number = boost::lexical_cast<uint64_t>(optarg);
      }
      catch (boost::bad_lexical_cast&) {
        std::cerr << "-n option should be an integer.";
        return 1;
      }
      break;
    default:
      usage();
      break;
    }
  }

  argc -= optind;
  argv += optind;

  if (argc != 0)
    usage();

  if (isAuto && isRead)
    usage();

  generator.run();
  return 0;
}

} // namespace repo

int
main(int argc, char** argv)
{
  try {
    return repo::main(argc, argv);
  }
  catch (std::exception& e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
    return 2;
  }
}
