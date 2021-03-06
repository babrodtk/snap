# SNAP: Servere Nuclear Accident Programme
# Copyright (C) 1992-2017   Norwegian Meteorological Institute
# 
# This file is part of SNAP. SNAP is free software: you can 
# redistribute it and/or modify it under the terms of the 
# GNU General Public License as published by the 
# Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#
package Snap;

use strict;
use warnings;

require Exporter;
use vars qw($VERSION @ISA @EXPORT @EXPORT_OK %EXPORT_TAGS $DEBUG);

$VERSION = 0.1;

@ISA = qw(Exporter);

$DEBUG = 0;

%EXPORT_TAGS = (
    'all' => [
        qw(
            put_statusfile
            system_ppi
            create_ppi_dir
            )
    ]
);
@EXPORT_OK = (@{$EXPORT_TAGS{'all'}});
@EXPORT    = qw();

sub put_statusfile {
    my ($smsdirs, $remote_hosts_and_users, $remote_ftp_dir, $run_ident, $model, $status_number) = @_;

    print qq[Run put_statusfile, $run_ident, $model, $status_number\n];

    # Create timestamp
    my ($min, $hour, $mday, $mon, $year) = (gmtime())[1, 2, 3, 4, 5];
    $year += 1900;
    $mon  += 1;
    $min  =~ s/^(\d)$/0$1/;
    $hour =~ s/^(\d)$/0$1/;
    $mday =~ s/^(\d)$/0$1/;
    $mon  =~ s/^(\d)$/0$1/;
    my $timestamp = "$year" . "$mon" . "$mday" . "$hour" . "$min";
    my $text      = "Unknown message number";

    if ($status_number == 101 && $run_ident eq q[ARGOS]) {
        open(TFILE, $smsdirs->{data} . "/snap.time") or (die "OPEN ERROR: snap.time, $!");
        while (<TFILE>) {
            chomp;
            my @parts = split(/\s+/);
            if ($#parts >= 4) {
                $text = "$parts[0]" . "$parts[1]" . "$parts[2]" . "$parts[3]" . "$parts[4]" . ":Running DERMA";
            }
        }
        close(TFILE);
    } elsif ($status_number == 101) {
        $text = $timestamp . q[ running];
    }

    if ($status_number == 100) {$text = ":Getting ARGOS data from server";}
    if ($status_number == 200) {$text = ":Finished getting ARGOS-data from server";}
    if ($status_number == 201) {$text = ":Finished running ${model}";}
    if ($status_number == 202) {$text = ":Finished extracting ${model} data for ARGOS";}
    if ($status_number == 401) {$text = ":$run_ident" . "_${model}_input does not exist";}
    if ($status_number == 402) {$text = ":$run_ident" . "_${model}_iso does not exist";}
    if ($status_number == 403) {$text = ":$run_ident" . "_${model}_src does not exist";}
    if ($status_number == 404) {$text = ":Inconsistent isotope identification (isotop-navn)";}
    if ($status_number == 408) {$text = ":Initial time not covered by NWP database";}
    if ($status_number == 409) {$text = ":${model} output data do not exist";}

    my $message = "$status_number" . ":" . "$timestamp" . ":" . "$text";

    print "\nSTATUS MESSAGE: $message\n\n";

    my $statusfile = "$run_ident" . "_${model}_status";

    open(STATUS, ">$statusfile") or die "OPEN ERROR: $statusfile";
    print STATUS ("$message\n");
    close(STATUS);

    my $sftpinp = "sftp.input";
    open(SFTPINP, ">$sftpinp") or die "OPEN ERROR: $sftpinp";
    print SFTPINP ("cd $remote_ftp_dir\n");
    print SFTPINP ("put $statusfile\nquit\n");
    close(SFTPINP);

    # Send result to remote host (both)
    foreach my $dest (@{$remote_hosts_and_users}) {
        my @command = ('sftp', '-b', $sftpinp, "$dest->{user}\@$dest->{host}");
        if ($DEBUG) {
            print STDERR join (" ", @command), "\n";
        } else {
            system("@command");
        }
    }
}

sub system_ppi {
    my ($remote_hosts_and_users, $command) = @_;
    my $ppiuser = $remote_hosts_and_users->[0]{'PPIuser'} or die "missing PPIuser\n";
    my $ppihost = $remote_hosts_and_users->[0]{'PPIhost'} or die "missing PPIhost\n";;
    my $dircommand = $command;
    if (defined $remote_hosts_and_users->[0]{'PPIdir'}) {
        $dircommand = 'cd ' . $remote_hosts_and_users->[0]{'PPIdir'} . ' && ' . $command;
    }
    print STDERR $dircommand, "\n" if $DEBUG;

    return system("ssh -l $ppiuser $ppihost '$dircommand'");
}

sub create_ppi_dir {
    my ($remote_hosts_and_users, $dirs) = @_;

    my $dir = undef;
    for my $d (@$dirs) {
        if (system_ppi($remote_hosts_and_users, "mkdir -p $d") == 0) {
            $dir = $d;
            last;
        }
    }
    return $dir;
}


1;
__END__

=head1 NAME

Snap - general functionality to run snap

=head1 SYNOPSIS

  use Snap qw(put_statusfile);


  put_statusfile( \%smsdirs, $remote_hosts_and_users, $remote_ftp_dir,
            $run_ident, $model, 409 )

=head1 DESCRIPTION

The snap modules provides a few methods to work with snap model output via ssh/ftp.
Set $Snap::DEBUG = 1 to avoid remote access, e.g. for testing.


=head2 put_statusfile(\%smsdirs, $remote_hosts_and_users, $remote_ftp_dir,
            $run_ident, $model, $status )

Write a status back to the sftp.

=over 4

=item \%smsdirs  directories used by sms, usually $smsdirs->{data}, {etc}, {work}

=item $remote_hosts_and_users

   [ { host => "$remote_host_alias",
       user => "$remote_user" } ]

=item $remote_ftp_dir a directory

=item $run_ident  the basename for the run

=item $model the model type, e.g. MLDP0, SNAP, SNAPGLOBAL, TRAJ, SNAP-BOMB

=item $status, a status-number

=over 8

=item 100: Getting ARGOS data from server

=item 200: Finished getting ARGOS-data from server

=item 201: Finished running

=item 202: Finished extracting data for ARGOS

=item 401: input does not exist

=item 402: *iso does not exist

=item 403: *_src does not exist

=item 404: Inconsistent isotope identification (isotop-navn)

=item 408: Initial time not covered by NWP database

=item 409: output data do not exist

=back

=back

=head2 system_ppi( $remote_hosts_and_users, $command )

Execute the command throw ssh on ppi, and return the system-return value.

=over 4

=item $remote_hosts_and_users This must contain the following fields

   [ { PPIhost => "$remote_host_alias",
       PPIuser => "$remote_user",
       PPIdir  => "$remote_working_directory"  # optional, defaults to remote home
      } ]



=back

=head2 create_ppi_dir( $remote_hosts_and_users, ['/lustre/storeA/dir', '/lustre/storeB/dir'] )

Create on of the directories from a list. Return directory-name of the one created.

=over 4

=item $remote_hosts_and_users This must contain the following fields, but not! PPIdir

   [ { PPIhost => "$remote_host_alias",
       PPIuser => "$remote_user" } ]

=back

=head1 AUTHOR

Heiko Klein, E<lt>H.Klein@met.noE<gt>

=cut
